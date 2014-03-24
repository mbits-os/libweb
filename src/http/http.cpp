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
#include <dom/parsers/encoding_db.hpp>
#include <http/http.hpp>
#include <utils.hpp>
#include <string.h>

#if defined WIN32
#define PLATFORM L"Win32"
#elif defined _MACOSX_
#define PLATFORM L"Mac OSX"
#elif defined ANDROID
#define PLATFORM L"Android"
#elif defined __GNUC__
#define PLATFORM "UNIX"
#endif

#include <string>
#include "curl_http.hpp"
#include <dom/parsers/xml.hpp>
#include <dom/parsers/html.hpp>

namespace http
{
	namespace impl
	{
		struct ContentData
		{
			char* content;
			size_t content_length;
			size_t capacity;
			ContentData()
				: content(nullptr)
				, content_length(0)
				, capacity(0)
			{
			}
			~ContentData()
			{
				delete [] content;
			}

			bool grow(size_t newsize)
			{
				if (capacity >= newsize)
					return true;

				auto copy_capacity = capacity;
				if (!copy_capacity)
					copy_capacity = 16;

				while (copy_capacity < newsize)
					copy_capacity <<= 1;

				auto tmp = new (std::nothrow) char[copy_capacity];
				if (!tmp)
					return false;

				if (content)
					memcpy(tmp, content, content_length);

				delete [] content;
				content = tmp;
				capacity = copy_capacity;
				return true;
			}

			bool append(const void* data, size_t len)
			{
				if (data && len)
				{
					if (!grow(content_length + len))
						return false;

					memcpy(content + content_length, data, len);
					content_length += len;
				}
				return true;
			}

			void clear()
			{
				delete [] content;
				content = nullptr;
				content_length = 0;
				capacity = 0;
			}
		};

		class XmlHttpRequest
			: public http::XmlHttpRequest
			, public http::HttpCallback
			, public std::enable_shared_from_this<XmlHttpRequest>
		{
			ONREADYSTATECHANGE handler;
			void* handler_data;

			HTTP_METHOD http_method;
			std::string url;
			bool async;
			Headers request_headers;
			READY_STATE ready_state;
			ContentData body;

			int http_status;
			std::string reason;
			Headers response_headers;
			ContentData response;
			dom::DocumentPtr doc;

			bool send_flag, done_flag, debug;
			HttpEndpointPtr m_endpoint;

			bool m_followRedirects;
			size_t m_redirects;

			bool m_wasRedirected;
			std::string m_finalLocation;

			void onReadyStateChange()
			{
				if (handler)
					handler(this, handler_data);
			}
			bool rebuildDOM(const std::string& mimeType, const std::string& encoding);
			bool rebuildDOM(const char* forcedType = nullptr);
			bool parseXML(const std::string& encoding);
			bool parseHTML(const std::string& encoding);

			void clear_response()
			{
				http_status = 0;
				reason.clear();
				response_headers.clear();
				response.clear();
				doc.reset();
				m_wasRedirected = false;
				m_finalLocation.clear();
			}
		public:

			XmlHttpRequest()
				: handler(nullptr)
				, handler_data(nullptr)
				, http_method(HTTP_GET)
				, async(true)
				, ready_state(UNSENT)
				, http_status(0)
				, send_flag(false)
				, done_flag(false)
				, debug(false)
				, m_followRedirects(true)
				, m_redirects(10)
				, m_wasRedirected(false)
			{
			}

			~XmlHttpRequest()
			{
				if (m_endpoint)
					m_endpoint->releaseEndpoint();
			}

			void onreadystatechange(ONREADYSTATECHANGE handler, void* userdata) override;
			READY_STATE getReadyState() const override;

			void open(HTTP_METHOD method, const std::string& url, bool async = true) override;
			void setRequestHeader(const std::string& header, const std::string& value) override;

			void setBody(const void* body, size_t length) override;
			void send() override;
			void abort() override;

			int getStatus() const override;
			std::string getStatusText() const override;
			std::string getResponseHeader(const std::string& name) const override;
			std::map<std::string, std::string> getResponseHeaders() const override;
			size_t getResponseTextLength() const override;
			const char* getResponseText() const override;
			dom::DocumentPtr getResponseXml() override;
			dom::DocumentPtr getResponseHtml() override;

			bool wasRedirected() const override;
			const std::string getFinalLocation() const override;

			void setDebug(bool debug) override;
			void setMaxRedirects(size_t redirects) override;
			void setShouldFollowLocation(bool follow) override;

			void onStart() override;
			void onError() override;
			void onFinish() override;
			size_t onData(const void* data, size_t count) override;
			void onFinalLocation(const std::string& location) override;
			void onHeaders(const std::string& reason, int http_status, const Headers& headers) override;

			void appendHeaders() override;
			std::string getUrl() override;
			void* getContent(size_t& length) override;
			bool getDebug() override;
			bool shouldFollowLocation() override;
			long getMaxRedirs() override;
		};

		void XmlHttpRequest::onreadystatechange(ONREADYSTATECHANGE handler, void* userdata)
		{
			//Synchronize on(*this);
			this->handler = handler;
			this->handler_data = userdata;
			if (ready_state != UNSENT)
				onReadyStateChange();
		}

		http::XmlHttpRequest::READY_STATE XmlHttpRequest::getReadyState() const
		{
			return ready_state;
		}

		void XmlHttpRequest::open(HTTP_METHOD method, const std::string& url, bool async)
		{
			abort();

			//Synchronize on(*this);

			send_flag = false;
			clear_response();
			request_headers.clear();

			http_method = method;
			this->url = url;
			this->async = async;

			ready_state = OPENED;
			onReadyStateChange();
		}

		void XmlHttpRequest::setRequestHeader(const std::string& header, const std::string& value)
		{
			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			auto _h = std::tolower(header);
			Headers::iterator _it = request_headers.find(_h);
			if (_it == request_headers.end())
			{
				if (_h == "accept-charset") return;
				if (_h == "accept-encoding") return;
				if (_h == "connection") return;
				if (_h == "content-length") return;
				if (_h == "cookie") return;
				if (_h == "cookie2") return;
				if (_h == "content-transfer-encoding") return;
				if (_h == "date") return;
				if (_h == "expect") return;
				if (_h == "host") return;
				if (_h == "keep-alive") return;
				if (_h == "referer") return;
				if (_h == "te") return;
				if (_h == "trailer") return;
				if (_h == "transfer-encoding") return;
				if (_h == "upgrade") return;
				if (_h == "user-agent") return;
				if (_h == "via") return;
				if (_h.substr(0, 6) == "proxy-") return;
				if (_h.substr(0, 4) == "sec-") return;
				request_headers[_h] = header +": " + value;
			}
			else
			{
				_it->second += ", " + value;
			}
		}

		void XmlHttpRequest::setBody(const void* body, size_t length)
		{
			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			this->body.clear();

			if (http_method != HTTP_POST) return;

			this->body.append(body, length);
		}

		void XmlHttpRequest::send()
		{
			abort();

			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			send_flag = true;
			done_flag = false;
			if (!m_endpoint)
				m_endpoint = http::GetEndpoint(shared_from_this());
			if (m_endpoint)
				m_endpoint->send(async);
		}

		void XmlHttpRequest::abort()
		{
			if (m_endpoint)
				m_endpoint->abort();
		}

		int XmlHttpRequest::getStatus() const
		{
			return http_status;
		}

		std::string XmlHttpRequest::getStatusText() const
		{
			return reason;
		}

		std::string XmlHttpRequest::getResponseHeader(const std::string& name) const
		{
			Headers::const_iterator _it = response_headers.find(std::tolower(name));
			if (_it == response_headers.end()) return std::string();
			return _it->second;
		}

		std::map<std::string, std::string> XmlHttpRequest::getResponseHeaders() const
		{
			return response_headers;
		}

		size_t XmlHttpRequest::getResponseTextLength() const
		{
			return response.content_length;
		}

		const char* XmlHttpRequest::getResponseText() const
		{
			return (const char*)response.content;
		}

		dom::DocumentPtr XmlHttpRequest::getResponseXml()
		{
			// Synchronize on (*this);

			if (ready_state != DONE || done_flag) return false;

			if (!doc && response.content && response.content_length)
				if (!rebuildDOM("text/xml"))
					return dom::DocumentPtr();

			return doc; 
		}

		dom::DocumentPtr XmlHttpRequest::getResponseHtml()
		{
			// Synchronize on (*this);

			if (ready_state != DONE || done_flag) return false;

			if (!doc && response.content && response.content_length)
				if (!rebuildDOM("text/html"))
					return dom::DocumentPtr();

			return doc; 
		}

		bool XmlHttpRequest::wasRedirected() const { return m_wasRedirected; }
		const std::string XmlHttpRequest::getFinalLocation() const { return m_finalLocation; }

		void XmlHttpRequest::setDebug(bool debug)
		{
			this->debug = debug;
		}

		void XmlHttpRequest::setMaxRedirects(size_t redirects)
		{
			m_redirects = redirects;
		}

		void XmlHttpRequest::setShouldFollowLocation(bool follow)
		{
			m_followRedirects = follow;
		}

		void XmlHttpRequest::onStart()
		{
			if (!body.content || !body.content_length)
				http_method = HTTP_GET;

			//onReadyStateChange();
		}

		void XmlHttpRequest::onError()
		{
			ready_state = DONE;
			done_flag = true;
			onReadyStateChange();
		}

		void XmlHttpRequest::onFinish()
		{
			ready_state = DONE;
			onReadyStateChange();
		}

		size_t XmlHttpRequest::onData(const void* data, size_t count)
		{
			//Synchronize on(*this);

			size_t ret = response.append(data, count) ? count : 0;
			if (ret)
			{
				if (ready_state == HEADERS_RECEIVED) ready_state = LOADING;
				if (ready_state == LOADING)          onReadyStateChange();
			}
			return ret;
		}

		void XmlHttpRequest::onFinalLocation(const std::string& location)
		{
			m_wasRedirected = true;
			m_finalLocation = location;
		}

		void XmlHttpRequest::onHeaders(const std::string& reason, int http_status, const Headers& headers)
		{
			//Synchronize on(*this);
			this->http_status = http_status;
			this->reason = reason;
			this->response_headers = headers;

			ready_state = HEADERS_RECEIVED;
			onReadyStateChange();
		}

		void XmlHttpRequest::appendHeaders()
		{
			if (m_endpoint)
			{
				for (auto&& pair : request_headers)
					m_endpoint->appendHeader(pair.second);
			}
		}

		std::string XmlHttpRequest::getUrl() { return url; }
		void* XmlHttpRequest::getContent(size_t& length) { if (http_method == HTTP_POST) { length = body.content_length; return body.content; } return nullptr; }
		bool XmlHttpRequest::getDebug() { return debug; }
		bool XmlHttpRequest::shouldFollowLocation() { return m_followRedirects; }
		long XmlHttpRequest::getMaxRedirs() { return m_redirects; }

		static void getMimeAndEncoding(std::string ct, std::string& mime, std::string& enc)
		{
			mime.empty();
			enc.empty();
			size_t pos = ct.find_first_of(';');
			mime = std::tolower(ct.substr(0, pos));
			if (pos == std::string::npos) return;
			ct = ct.substr(pos+1);

			while(1)
			{
				pos = ct.find_first_of('=');
				if (pos == std::string::npos) return;
				std::string cand = ct.substr(0, pos);
				ct = ct.substr(pos+1);
				size_t low = 0, hi = cand.length();
				while (cand[low] && isspace((unsigned char)cand[low])) ++low;
				while (hi > low && isspace((unsigned char)cand[hi-1])) --hi;

				if (std::tolower(cand.substr(low, hi - low)) == "charset")
				{
					pos = ct.find_first_of(';');
					enc = ct.substr(0, pos);
					low = 0; hi = enc.length();
					while (enc[low] && isspace((unsigned char)enc[low])) ++low;
					while (hi > low && isspace((unsigned char)enc[hi-1])) --hi;
					enc = enc.substr(low, hi-low);
					return;
				}
				pos = ct.find_first_of(';');
				if (pos == std::string::npos) return;
				ct = ct.substr(pos+1);
			};
		}

		bool XmlHttpRequest::rebuildDOM(const std::string& mime, const std::string& encoding)
		{
			if (mime == "" ||
				mime == "text/xml" ||
				mime == "application/xml" || 
				(mime.length() > 4 && mime.substr(mime.length()-4) == "+xml"))
			{
				return parseXML(encoding);
			}

			if (mime == "text/html")
			{
				return parseHTML(encoding);
			}

			return true;
		}

		bool XmlHttpRequest::rebuildDOM(const char* forcedType)
		{
			std::string mime;
			std::string enc;
			getMimeAndEncoding(getResponseHeader("Content-Type"), mime, enc);
			if (forcedType)
				return rebuildDOM(forcedType, enc);
			return rebuildDOM(mime, enc);
		}

		bool XmlHttpRequest::parseXML(const std::string& encoding)
		{
			doc = dom::parsers::xml::parseDocument(encoding, response.content, response.content_length);
			return !!doc;
		}

		bool XmlHttpRequest::parseHTML(const std::string& encoding)
		{
			doc = dom::parsers::html::parseDocument(encoding, response.content, response.content_length);
			return !!doc;
		}
	} // http::impl

	XmlHttpRequestPtr XmlHttpRequest::Create()
	{
		try {
			return std::make_shared<impl::XmlHttpRequest>();
		} catch(std::bad_alloc) {
			return nullptr;
		}
	}
}
