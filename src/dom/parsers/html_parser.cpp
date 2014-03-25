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
#include <dom/parsers/html.hpp>
#include <vector>
#include <utils.hpp>
#include <dom/dom.hpp>
#include <cstring>

namespace google
{
	#include <gumbo.h>
}

namespace utf8
{
	constexpr uint32_t unicodeReplacement{ 0x0000FFFD };
	inline size_t length(uint32_t utf32)
	{
		if (utf32 < 0x00000080) return 1;
		if (utf32 < 0x00000800) return 2;
		if (utf32 < 0x00010000) return 3;
		if (utf32 < 0x00200000) return 4;
		if (utf32 < 0x04000000) return 5;
		return 6;
	}

	inline void move(char*& dst, uint32_t& utf32)
	{
		*--dst = (utf32 & 0x3f) | 0x80;
		utf32 >>= 6;
	}

	inline void conv(std::string& utf8, uint32_t utf32)
	{
		static const uint32_t firstByteMark[7] = { 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc };

		char seq[6];
		if (utf32 & 0x80000000)
			utf32 = unicodeReplacement;

		auto bytes = length(utf32);
		char* dst = seq + bytes;
		switch (bytes)
		{
		case 6: move(dst, utf32);
		case 5: move(dst, utf32);
		case 4: move(dst, utf32);
		case 3: move(dst, utf32);
		case 2: move(dst, utf32);
		case 1: *--dst = utf32 | firstByteMark[bytes];
		}

		utf8.append(seq, bytes);
	}

	inline std::string fromUtf32(const uint32_t* string, size_t length)
	{
		size_t len = 0;
		auto c = string;
		for (size_t i = 0; i < length; ++i, ++c)
		{
			auto utf32 = *c;
			if (utf32 & 0x80000000)
				utf32 = unicodeReplacement;
			len += utf8::length(utf32);
		}

		std::string utf8;
		utf8.reserve(len + 1);

		c = string;
		for (size_t i = 0; i < length; ++i, ++c)
			conv(utf8, *c);

		return utf8;
	}

	inline std::string fromUtf32(uint32_t c)
	{
		return fromUtf32(&c, 1);
	}

	template <template <typename T, typename A> class C, typename A>
	inline std::string fromUtf32(const C<uint32_t, A>& c)
	{
		return fromUtf32(c.data(), c.size());
	}
}

namespace dom { namespace parsers { namespace html {

	struct TextConverter
	{
		virtual ~TextConverter() {}
		virtual std::string conv(const std::string&) = 0;
	};
	using TextConverterPtr = std::shared_ptr<TextConverter>;

	struct Identity : TextConverter
	{
		std::string conv(const std::string& s) override { return s; }
	};

	struct eXpatConverter : TextConverter
	{
		int map[256];

		std::string conv(const std::string& s) override
		{
			std::vector<uint32_t> data;

			data.reserve(s.length());
			for (auto&& c : s)
			{
				uint32_t utf32 = map[(unsigned char)c];
				if (utf32 < 0)
					utf32 = utf8::unicodeReplacement;
				data.push_back(utf32);
			}
			return utf8::fromUtf32(data);
		}

		static TextConverterPtr create(const std::string& cp)
		{
			auto conv = std::make_shared<eXpatConverter>();
			if (!conv)
				return nullptr;

			if (!loadCharset(cp, conv->map))
				return nullptr;

			return conv;
		}
	};

	class Parser : public parsers::Parser
	{
		dom::ElementPtr elem;
		dom::DocumentFragmentPtr container;
		std::string text;
		std::string encoding;
		TextConverterPtr converter;
		std::string conv(const std::string& s) { return converter->conv(s); }
		bool switchConv(std::string cp)
		{
			std::tolower(cp);
			if (converter && cp == encoding)
				return true;

			if (cp.empty() || cp == "utf-8" || cp == "utf8")
				converter = std::make_shared<Identity>();
			else
				converter = eXpatConverter::create(cp);

			encoding = cp;

			return !!converter;
		}

		inline void expand(const char* entity, const char* val)
		{
			std::string::size_type len = strlen(entity);
			std::string::size_type ntity = text.find(entity);
			while (ntity != std::string::npos)
			{
				text = text.substr(0, ntity) + val + text.substr(ntity + len);
				ntity = text.find(entity, ntity);
			}
		}

		inline void expandNumericals()
		{
			std::string::size_type ntity = text.find("&#");
			while (ntity != std::string::npos)
			{
				std::string::size_type ptr = ntity + 2;
				char hex = text[ptr];

				uint32_t utf32 = 0;
				if (hex == 'x' || hex == 'X')
				{
					++ptr;

					while (isxdigit((unsigned char)text[ptr]))
					{
						switch (text[ptr])
						{
						case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
							utf32 *= 16;
							utf32 += text[ptr] - 'A' + 10;
							break;
						case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
							utf32 *= 16;
							utf32 += text[ptr] - 'a' + 10;
							break;
						default:
							utf32 *= 10;
							utf32 += text[ptr] - '0';
						};

						++ptr;
					}
				}
				else
				while (isdigit((unsigned char)text[ptr]))
				{
					utf32 *= 10;
					utf32 += text[ptr] - '0';
					++ptr;
				}

				if (text[ptr] == ';')
				{
					++ptr;
					if (utf32 == 0xA0) utf32 = ' ';
					text = text.substr(0, ntity) + utf8::fromUtf32(utf32) + text.substr(ptr);
				}
				else
					ntity += 2; //to skip this &# in the next find

				ntity = text.find("&#", ntity);
			}
		};

		inline void addText()
		{
			if (text.empty()) return;

			text = conv(text);

			expand("&nbsp;", " ");
			expand("&quot;", "\"");
			expand("&lt;", "<");
			expand("&gt;", ">");
			expandNumericals();
			expand("&amp;", "&");

			dom::TextPtr node = doc->createTextNode(text);
			if (node)
			{
				if (elem)
					elem->appendChild(node);
				else
					container->appendChild(node);
			}
			text.clear();
		}

		dom::DocumentPtr doc;

	public:

		Parser()
			: converter(std::make_shared<Identity>())
		{
		}

		bool create(const std::string& cp)
		{
			doc = dom::Document::create();
			if (!doc)
				return false;
			container = doc->createDocumentFragment();
			if (!container)
				return false;
			return switchConv(cp);
		}

		template <typename T>
		struct gumbo_vector
		{
			typedef T* iterator;
			iterator data;
			size_t length;
			gumbo_vector(google::GumboVector& v) : data((iterator)v.data), length(v.length) {}
			iterator begin() const { return data; }
			iterator end() const { return data + length; }
		};

		static std::string gumbo_string(const google::GumboStringPiece& text)
		{
			return std::string(text.data, text.length);
		}

		static bool textFromGumbo(const std::shared_ptr<dom::ParentNode>& parent, google::GumboText* text)
		{
			return parent->append(text->text);
		}

		static bool elementFromGumbo(const std::shared_ptr<dom::ParentNode>& parent, google::GumboElement* element)
		{
			if (!element->original_tag.length) // algorithmical
			{
				for (auto&& node : gumbo_vector<google::GumboNode*>{ element->children })
				{
					if (!fromGumbo(parent, node))
						return false;
				}
				return true;
			}

			std::string tagName = element->tag_namespace == google::GUMBO_NAMESPACE_HTML ?
				google::gumbo_normalized_tagname(element->tag) : gumbo_string(element->original_tag);

			auto doc = parent->ownerDocument();
			if (!doc)
				return false;
			auto e = doc->createElement(tagName);
			if (!e)
				return false;
			parent->append(e);

			for (auto&& attr : gumbo_vector<google::GumboAttribute*>{ element->attributes })
			{
				e->setAttribute(attr->name, attr->value);
			}

			for (auto&& node : gumbo_vector<google::GumboNode*>{ element->children })
			{
				if (!fromGumbo(e, node))
					return false;
			}

			return true;
		}

		static bool fromGumbo(const std::shared_ptr<dom::ParentNode>& parent, google::GumboNode* node)
		{
			if (!node)
				return false;

			switch (node->type)
			{
			case google::GUMBO_NODE_ELEMENT:
				return elementFromGumbo(parent, &node->v.element);
			case google::GUMBO_NODE_TEXT:
			case google::GUMBO_NODE_CDATA:
			case google::GUMBO_NODE_WHITESPACE:
				return textFromGumbo(parent, &node->v.text);
			}

			return true;
		}

		static bool breakContentType(std::string content, std::string& charset)
		{
			std::string::size_type semi = content.find(';');
			std::string::size_type len = content.length();
			while (semi != std::string::npos)
			{
				++semi;
				while (semi < len && isspace((unsigned char)content[semi])) ++semi;
				if (std::tolower(content.substr(semi, 7)) == "charset")
				{
					semi += 7;
					while (semi < len && isspace((unsigned char)content[semi])) ++semi;
					if (semi < len && content[semi] == '=')
					{
						++semi;
						std::string::size_type last = content.find(';', semi);
						if (last == std::string::npos)
							charset = content.substr(semi);
						else
							charset = content.substr(semi, last - semi);
						std::trim(charset);
						std::tolower(charset);

						return true;
					}
				}
				semi = content.find(';', semi);
			}

			return false;
		}

		static bool findEncoding(google::GumboElement* e, std::string& out)
		{
			if (e->tag == google::GUMBO_TAG_META)
			{
				auto attr = google::gumbo_get_attribute(&e->attributes, "http-equiv");
				if (attr && std::tolower(attr->value) == "content-type")
				{
					attr = google::gumbo_get_attribute(&e->attributes, "content");
					if (attr)
						return breakContentType(attr->value, out);
				}
			}

			for (auto&& node : gumbo_vector<google::GumboNode*>{ e->children })
			{
				if (node->type != google::GUMBO_NODE_ELEMENT)
					continue;

				if (findEncoding(&node->v.element, out))
					return true;
			}

			return false;
		}

		bool supportsChunks() const override { return false; }
		bool onData(const void* begin, size_t size) override
		{
			auto output = google::gumbo_parse_with_options(&google::kGumboDefaultOptions, (const char*)begin, size);
			if (!output)
				return false;

			std::string encoding, newText;
			auto found = findEncoding(&output->root->v.element, encoding);
			if (found && encoding != this->encoding)
			{
				// restart
				google::gumbo_destroy_output(&google::kGumboDefaultOptions, output);

				switchConv(encoding);
				newText = conv(std::string((const char*)begin, size));

				output = google::gumbo_parse_with_options(&google::kGumboDefaultOptions, newText.c_str(), newText.length());
				if (!output)
					return false;
			}

			auto success = fromGumbo(container, output->root);
			google::gumbo_destroy_output(&google::kGumboDefaultOptions, output);

			if (!success)
				return false;

			auto list = container->childNodes();
			if (list)
			{
				if (list->length() == 1)
				{
					auto e = list->element(0);
					if (e)
					{
						doc->setDocumentElement(e);
						return true;
					}
				}

				doc->setFragment(container);
			}
		
			return true;
		}

		DocumentPtr onFinish() override
		{
			return doc;
		}
	};

	ParserPtr create(const std::string& encoding)
	{
		try
		{
			auto parser = std::make_shared<Parser>();
			if (!parser->create(encoding))
				return nullptr;

			return parser;
		}
		catch (std::bad_alloc&)
		{
			return nullptr;
		}
	}

	void serializeChildren(OutStream& stream, const NodePtr& node)
	{
		for (auto&& child : list_nodes(node->childNodes()))
			serialize(stream, child);
	}

	static const char* closed[] = {
		"area",
		"base",
		"basefont",
		"br",
		"col",
		"frame",
		"hr",
		"img",
		"input",
		"isindex",
		"link",
		"meta",
		"param",
		"nextid",
		"bgsound",
		"embed",
		"keygen",
		"spacer",
		"wbr"
	};

	void serializeElement(OutStream& stream, const ElementPtr& e)
	{
		auto tag = std::tolower(e->tagName());

		stream << '<' << tag;

		for (auto&& att : dom::list_atts(e->getAttributes()))
		{
			if (!att) continue;

			stream << ' '<< std::tolower(att->name()) << "=\"" << url::htmlQuotes(att->value()) << '"';
		}

		for (auto&& name : closed)
		{
			if (tag == name)
			{
				stream << "/>";
				return;
			}
		}

		stream << '>';
		serializeChildren(stream, e);
		stream << "</" << tag << '>';
	}

	void serializeText(OutStream& stream, const TextPtr& t)
	{
		stream << url::htmlQuotes(t->data());
	}

	void serialize(OutStream& stream, const dom::NodePtr& node)
	{
		if (!node)
			return;

		auto type = node->nodeType();
		switch (type)
		{
		case dom::TEXT_NODE:
			return serializeText(stream, std::static_pointer_cast<dom::Text>(node));
		case dom::ELEMENT_NODE:
			return serializeElement(stream, std::static_pointer_cast<dom::Element>(node));
		}

		serializeChildren(stream, node);
	}

}}}
