/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"
#include <mail/mail.hpp>
#include <crypt.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <future>
#include <thread>
#include <filesystem.hpp>
#include <iomanip>
#include <cctype>
#include <mt.hpp>

//#define DEBUG_SMTP
//#define DEBUG_SMTP_FULL

#if WIN32
#include <direct.h>
#else
#include <unistd.h>
#define _getcwd getcwd
#endif

namespace mail
{
	struct configuration: public mt::AsyncData
	{
		std::string server, user, password, mailbox, machine;
	};

	static configuration g_smtp;

	void MimePart::echo()
	{
		if (m_fileName.empty())
			addHeader("Content-Disposition", contentDisposition());
		else
			addHeader("Content-Disposition", contentDisposition() + ("; filename=" + m_fileName));

		if (m_buffer)
		{
			echoBody();

			std::ostringstream str;
			str << m_buffer->size();

			addHeader("Content-Length", str.str());

			echoHeaders();

			m_downstream->put("\r\n");
			m_buffer->flush();
			m_downstream->put("\r\n");

			return;
		}

		echoHeaders();
		m_downstream->put("\r\n");
		echoBody();
		m_downstream->put("\r\n");
	}

	void MimePart::echoHeaders()
	{
		for (auto& head : m_headers)
		{
			m_downstream->put(head.first);
			m_downstream->put(": ");
			m_downstream->put(head.second);
			m_downstream->put("\r\n");
		}
	}

	static std::string makeCID(size_t part, const std::string& machine)
	{
		static size_t globalPartCounter = 0;
		std::ostringstream out;
		out << part << '.' << ++globalPartCounter << '.' << _getpid() << '.' << tyme::now() << '@' << machine;
		return std::move(out.str());
	}

#ifdef WIN32
#define SEP '\\'
#else
#define SEP '/'
#endif

	ImagePart::ImagePart(size_t part, const std::string& machine, const std::string& mime, const filesystem::path& path)
		: MimePart(mime, "base64")
		, m_cid(std::move(makeCID(part, machine)))
		, m_path(path)
	{
		setFileName(m_path.filename().string());
		addHeader("Content-ID", "<" + m_cid + ">");
	}

	void ImagePart::echoBody()
	{
		FilterPtr filter = sink();
		if (!filter)
			return;

		auto path = filesystem::canonical(m_path).native();
		std::ifstream file(path, std::ios::in | std::ios::binary);
		char c;
		while (file.get(c))
			filter->put(c);

		filter->bodyEnd();
	}

	void Multipart::echoBody()
	{
		auto raw = downstream();
		auto copy = m_parts;
		decltype(copy.size()) prev_size = 0;
		while (!copy.empty())
		{
			for (auto& part : copy)
			{
				raw->put("--");
				raw->put(m_boundary);
				raw->put("\r\n");
				part->echo();
			}

			prev_size += copy.size();
			auto c_size = prev_size;
			auto o_size = m_parts.size();
			auto start = c_size < o_size ? c_size : o_size;

			auto begin = m_parts.begin();
			std::advance(begin, start);
			copy.assign(begin, m_parts.end());
		}

		raw->put("--");
		raw->put(m_boundary);
		raw->put("--\r\n\r\n");

		raw->bodyEnd();
	}

	void Multipart::pipe(const FilterPtr& downstream)
	{
		MimePart::pipe(downstream);
		for (auto part : m_parts)
		{
			part->pipe(downstream);
		}
	}

	template <typename Upstream>
	struct MethodProducer : TextProducer
	{
		std::shared_ptr<Upstream> m_upstream;
		void (Upstream::* m_method)(const FilterPtr&);
		MethodProducer(const std::shared_ptr<Upstream>& upstream, void (Upstream::* method)(const FilterPtr&))
			: m_upstream(upstream)
			, m_method(method)
		{
		}

		void echo(const FilterPtr& downstream) override
		{
			(m_upstream.get()->*m_method)(downstream);
		}
	};

	template <typename Upstream>
	TextProducerPtr make_producer(const std::shared_ptr<Upstream>& upstream, void (Upstream::* method)(const FilterPtr&))
	{
		return std::make_shared<MethodProducer<Upstream>>(upstream, method);
	}

	Message::Message(const std::string& subject, const std::string& boundary, const std::string& machine, const MessageProducerPtr& producer)
		: Multipart("multipart/related", boundary)
		, m_partsSoFar(3)
		, m_subject(subject)
		, m_machine(machine)
		, m_text(std::make_shared<TextPart>("text/plain; charset=utf-8", make_producer(producer, &MessageProducer::echoPlain)))
		, m_html(std::make_shared<TextPart>("text/html; charset=utf-8", make_producer(producer, &MessageProducer::echoHtml)))
		, m_msg(std::make_shared<Multipart>("multipart/alternative", boundary + "_message"))
	{
		char now[100];
		tyme::strftime(now, "%a, %d %b %Y %H:%M:%S GMT", tyme::gmtime(tyme::now()));

		Crypt::md5_t hash;
		Crypt::md5(now, hash);

		std::string msgId;
		msgId.reserve(machine.size() + sizeof(hash) + 3);
		msgId.push_back('<');
		msgId.append(hash, sizeof(hash)-1);
		msgId.push_back('@');
		msgId += machine;
		msgId.push_back('>');

		addHeader("Date", now);
		addHeader("Message-ID", msgId);
		addHeader("X-Mailer", "reedr/1.0");
		addHeader("MIME-Version", "1.0");
		m_msg->push_back(m_text);
		m_msg->push_back(m_html);
		push_back(m_msg);
	}

	ImagePartPtr Message::createInlineImage(const std::string& mime, const std::string& path)
	{
		auto img = std::make_shared<ImagePart>(++m_partsSoFar, m_machine, mime, path);
		img->pipe(m_downstream);
		push_back(img);
		return img;
	}

	void Message::echo()
	{
		echoHeaders();
		downstream()->put("\r\n");
		echoBody();
	}

	std::string join(const std::vector<Address>& addresses)
	{
		std::string out;
		bool first = true;
		for (auto& address : addresses)
		{
			if (first) first = false;
			else out += ", ";
			out += address.format();
		}
		return std::move(out);
	}

	template <typename Pred>
	std::string join(const std::vector<Address>& addresses, Pred pred)
	{
		std::string out;
		bool first = true;
		for (auto& address : addresses)
		{
			if (first) first = false;
			else out += ", ";
			out += pred(address);
		}
		return std::move(out);
	}

	void Message::post()
	{
		//FLOG << "[SMTP] Generating the message.";
		addHeader("Subject", Base64::header(m_subject));
		addHeader("From", m_from.format());
		addHeader("To", join(m_to));
		if (!m_cc.empty()) addHeader("Cc", join(m_cc));
		if (!m_bcc.empty()) addHeader("Bcc", join(m_bcc));
		echo();
		m_downstream->close();
		//FLOG << "[SMTP] Message generated";
	}

	void Message::setFrom(const std::string& name)
	{
		Synchronize on(g_smtp);
		m_from.assign(name, g_smtp.mailbox + "@" + g_smtp.machine);
	}

	std::string Message::getSender() const { return m_from.mail(); }

	std::vector<std::string> Message::getRecipients() const
	{
		std::vector<std::string> tos;
		tos.reserve(m_to.size() + m_cc.size() + m_bcc.size());
		std::transform(
			m_to.begin(), m_to.end(),
			std::back_inserter(tos),
			[](const Address& a) { return a.mail(); }
		);
		std::transform(
			m_cc.begin(), m_cc.end(),
			std::back_inserter(tos),
			[](const Address& a) { return a.mail(); }
		);
		std::transform(
			m_bcc.begin(), m_bcc.end(),
			std::back_inserter(tos),
			[](const Address& a) { return a.mail(); }
		);
		return tos;
	}

	static bool readProps(const filesystem::path& path, std::map<std::string, std::string>& props)
	{
		std::ifstream ini(path.native());
		if (!ini.is_open())
			return false;

		std::string line;
		while (!ini.eof())
		{
			ini >> line;
			std::string::size_type enter = line.find('=');
			if (enter != std::string::npos)
				props[line.substr(0, enter)] = line.substr(enter + 1);
		}
		return true;
	}

	std::string read(const std::map<std::string, std::string>& props, const std::string& key)
	{
		auto it = props.find(key);
		if (it == props.end())
			return std::string();

		return it->second;
	}

	void PostOffice::init(const filesystem::path& ini)
	{
		std::map<std::string, std::string> props;
		if (!readProps(ini, props))
			return;

		g_smtp.server = read(props, "server");
		g_smtp.user = read(props, "user");
		g_smtp.password = read(props, "password");
		g_smtp.mailbox = read(props, "mailbox");
		g_smtp.machine = read(props, "machine");

#if 0
		if (g_smtp.server.empty())
			FLOG << "[SMTP] Config is missing server key";

		if (g_smtp.user.empty())
			FLOG << "[SMTP] Config is missing user key";
#endif

		if (g_smtp.mailbox.empty())
		{
			g_smtp.mailbox = g_smtp.user.substr(0, g_smtp.user.find('@'));
		}

		if (g_smtp.machine.empty())
		{
			g_smtp.machine = g_smtp.server.substr(0, g_smtp.server.find(':'));
		}

		//FLOG << "[SMTP] e-mail: " << g_smtp.mailbox << "@" << g_smtp.machine << " (via " << g_smtp.server << ')';
	}

	void PostOffice::reload(const filesystem::path& ini)
	{
		Synchronize on(g_smtp);
		init(ini);
	}

	void PostOffice::post(const MessagePtr& ptr, bool async)
	{
		std::packaged_task<int()> task([ptr]()
		{
			return send(ptr->getSender(), ptr->getRecipients(), ptr);
		});
		auto future = task.get_future();
		std::thread thread(std::move(task));
		if (async)
			thread.detach();
		else
			thread.join();
	}

	class Curl
	{
		CURL* m_curl;
		curl_slist * m_recipients;
		filter::FileDescriptor m_fd;

		static size_t fread(void *buffer, size_t size, size_t count, Curl* curl)
		{
#ifdef DEBUG_SMTP_FULL
			FLOG << "Required " << count << " blocks of " << size << " bytes each (for " << curl << ")";
#endif
			return curl->m_fd.read(buffer, size * count) / size;
		}

	public:
		Curl();
		~Curl() { close(); }
		bool open();
		void close();
		void closeFd();
		void setSender(const std::string& from);
		void setRecipients(const std::vector<std::string>& to);
		void pipe(filter::FileDescriptor&& fd);
		CURLcode transfer();
	};

	int PostOffice::send(const std::string& from, const std::vector<std::string>& to, const MessagePtr& ptr)
	{
		//FLOG << "[SMTP] Sending a message...";
		if (to.empty())
			return -1;

		Curl smtp;
		if (!smtp.open())
		{
			//FLOG << "[SMTP] Could not open SMTP object";
			return -1;
		}

		smtp.setSender(from);
		smtp.setRecipients(to);

		filter::Pipe pipe;
		if (!pipe.open())
		{
			//FLOG << "[SMTP] Could not create pipe.";
			return -1;
		}

		smtp.pipe(std::move(pipe.reader));
		ptr->pipe(std::make_shared<filter::FdFilter>(std::move(pipe.writer)));

		auto thread = std::thread([ptr]() { ptr->post(); });

		auto res = smtp.transfer();

		//if (res != CURLE_OK)
		//	FLOG << "[SMTP] curl_easy_perform() failed: " << curl_easy_strerror(res);

		smtp.closeFd();
		thread.join();
		//FLOG << "[SMTP] Message sent...";
		return res;
	}

	Curl::Curl()
		: m_curl(nullptr)
		, m_recipients(nullptr)
		, m_fd(0)
	{
	}

#ifdef DEBUG_SMTP
	static void dump(const char *text, unsigned char *ptr, size_t size)
	{
		constexpr unsigned int width = 0x10;

		FLOG << text << ", " << size << " bytes";

		for (size_t i = 0; i < size; i += width)
		{
			FastCGI::ApplicationLog log{ __FILE__, __LINE__ };

			log << std::setw(4) << std::setfill('0') << std::hex << i << ": ";

			/* hex not disabled, show it */
			for (size_t c = 0; c < width; c++)
			{
				if (i + c < size)
					log << std::setw(2) << std::setfill('0') << (int)ptr[i + c] << ' ';
				else
					log << "   ";
			}

			log << std::dec;

			for (size_t c = 0; (c < width) && (i + c < size); c++)
			{
				auto _c = ptr[i + c];
				if (std::isprint(_c))
					log << _c;
				else
					log << '.';
			}
		}
	}

	static int my_trace(CURL *, curl_infotype type, char *data, size_t size, void *)
	{
#ifdef DEBUG_SMTP_FULL
		const char *text;
#endif

		switch (type) {
		case CURLINFO_TEXT:
			if (data && data[strlen(data) - 1] == '\n')
				FLOG << "== Info: " << std::string(data, data + (strlen(data) - 1));
			else
				FLOG << "== Info: " << data;
		default: /* in case a new one is introduced to shock us */
			return 0;
#ifdef DEBUG_SMTP_FULL
		case CURLINFO_HEADER_OUT:
			text = "=> Send header";
			break;
		case CURLINFO_DATA_OUT:
			text = "=> Send data";
			break;
		case CURLINFO_SSL_DATA_OUT:
			text = "=> Send SSL data";
			break;
		case CURLINFO_HEADER_IN:
			text = "<= Recv header";
			break;
		case CURLINFO_DATA_IN:
			text = "<= Recv data";
			break;
		case CURLINFO_SSL_DATA_IN:
			text = "<= Recv SSL data";
			break;
#endif
		}

#ifdef DEBUG_SMTP_FULL
		dump(text, (unsigned char *)data, size);
		return 0;
#endif
	}
#endif

	bool Curl::open()
	{
		m_curl = curl_easy_init();

		if (!m_curl)
			return false;

		Synchronize on(g_smtp);

		curl_easy_setopt(m_curl, CURLOPT_URL, ("smtp://" + g_smtp.server).c_str());
		curl_easy_setopt(m_curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);

		if (!g_smtp.user.empty() && !g_smtp.password.empty())
		{
			curl_easy_setopt(m_curl, CURLOPT_USERNAME, g_smtp.user.c_str());
			curl_easy_setopt(m_curl, CURLOPT_PASSWORD, g_smtp.password.c_str());
		}

#ifdef DEBUG_SMTP
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, my_trace);
		curl_easy_setopt(m_curl, CURLOPT_DEBUGDATA, nullptr);
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1l);
#endif

		return true;
	}

	void Curl::close()
	{
		curl_slist_free_all(m_recipients);
		curl_easy_cleanup(m_curl);
	}

	void Curl::closeFd()
	{
		m_fd.close();
	}

	void Curl::setSender(const std::string& from)
	{
		curl_easy_setopt(m_curl, CURLOPT_MAIL_FROM, from.c_str());
	}

	void Curl::setRecipients(const std::vector<std::string>& to)
	{
		for (auto& addr : to)
			m_recipients = curl_slist_append(m_recipients, addr.c_str());
		curl_easy_setopt(m_curl, CURLOPT_MAIL_RCPT, m_recipients);
	}

	void Curl::pipe(filter::FileDescriptor&& fd)
	{
		m_fd = std::move(fd);
		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, fread);
		curl_easy_setopt(m_curl, CURLOPT_INFILE, this);
	}

	CURLcode Curl::transfer()
	{
		return curl_easy_perform(m_curl);
	}

	MessagePtr PostOffice::newMessage(const std::string& subject, const MessageProducerPtr& producer)
	{
		if (!producer)
			return nullptr;

		char now[100];
		tyme::strftime(now, "%a, %d %b %Y %H:%M:%S GMT", tyme::gmtime(tyme::now()));

		Crypt::md5_t boundary;
		Crypt::md5(now, boundary);

		Synchronize on(g_smtp);

		auto msg = std::make_shared<Message>(subject, boundary, g_smtp.machine, producer);
		producer->setMessage(msg);
		return msg;
	}

}
