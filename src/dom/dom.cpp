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
#include <dom/dom.hpp>
#include <dom/dom_xpath.hpp>
#include <vector>
#include <iterator>
#include <string.h>
#include <dom/parsers/expat.hpp>

namespace dom {

	class DOMParser: public xml::ExpatBase<DOMParser>
	{
		dom::XmlElementPtr elem;
		std::string text;

		void addText()
		{
			if (text.empty()) return;
			if (elem)
				elem->appendChild(doc->createTextNode(text));
			text.clear();
		}
	public:

		dom::XmlDocumentPtr doc;

		bool create(const char* cp)
		{
			doc = dom::XmlDocument::create();
			if (!doc) return false;
			return xml::ExpatBase<DOMParser>::create(cp);
		}

		void onStartElement(const XML_Char *name, const XML_Char **attrs)
		{
			addText();
			auto current = doc->createElement(name);
			if (!current) return;

			for (; *attrs; attrs += 2)
				current->setAttribute(attrs[0], attrs[1]);

			if (elem)
				elem->appendChild(current);
			else
				doc->setDocumentElement(current);
			elem = current;
		}

		void onEndElement(const XML_Char *name)
		{
			addText();
			if (!elem) return;
			dom::XmlNodePtr node = elem->parentNode();
			elem = std::static_pointer_cast<dom::XmlElement>(node);
		}

		void onCharacterData(const XML_Char *pszData, int nLength)
		{
			text += std::string(pszData, nLength);
		}
	};

	XmlDocumentPtr XmlDocument::fromFile(const char* path)
	{
		DOMParser parser;
		if (!parser.create(nullptr)) return nullptr;
		parser.enableElementHandler();
		parser.enableCharacterDataHandler();

		FILE* f = fopen(path, "rb");
		if (!f)
			return nullptr;

		char buffer[8192];
		size_t read;
		while ((read = fread(buffer, 1, sizeof(buffer), f)) > 0)
		{
			if (!parser.parse(buffer, read, false))
			{
				fclose(f);
				return nullptr;
			}
		}
		fclose(f);

		if (!parser.parse(buffer, 0))
			return nullptr;

		return parser.doc;
	}

	void Print(const dom::XmlNodeListPtr& subs, bool ignorews, size_t depth)
	{
		if (subs)
		{
			size_t count = subs->length();
			for(size_t i = 0; i < count; ++i)
				Print(subs->item(i), ignorews, depth);
		}
	}

	template <typename T>
	inline std::string qName(std::shared_ptr<T> ptr)
	{
		QName qname = ptr->nodeQName();
		if (qname.nsName.empty()) return qname.localName;
		return "{" + qname.nsName + "}" + qname.localName;
	}

	void Print(const dom::XmlNodePtr& node, bool ignorews, size_t depth)
	{
		dom::XmlNodeListPtr subs = node->childNodes();

		NODE_TYPE type = node->nodeType();
		std::string out;
		for (size_t i = 0; i < depth; ++i) out += "    ";

		if (type == TEXT_NODE)
		{
			std::string val = node->nodeValue();
			if (ignorews)
			{
				size_t lo = 0, hi = val.length();
				while (lo < hi && val[lo] && isspace((unsigned char)val[lo])) lo++;
				while (lo < hi && isspace((unsigned char)val[hi-1])) hi--;
				val = val.substr(lo, hi - lo);
				if (val.empty()) return;
			}
			if (val.length() > 80)
				val = val.substr(0, 77) + "[...]";
			out += "# " + val;
		}
		else if (type == ELEMENT_NODE)
		{
			std::string sattrs;
			auto e = std::static_pointer_cast<dom::XmlElement>(node);
			dom::XmlNodeListPtr attrs;
			if (e) attrs = e->getAttributes();
			if (attrs)
			{
				size_t count = attrs->length();
				for(size_t i = 0; i < count; ++i)
				{
					dom::XmlNodePtr node = attrs->item(i);
					sattrs += " " + qName(node) + "='" + node->nodeValue() + "'";
				}
			}

			if (subs && subs->length() == 1)
			{
				dom::XmlNodePtr sub = subs->item(0);
				if (sub && sub->nodeType() == TEXT_NODE)
				{
					out += qName(node);
					if (!sattrs.empty())
						out += "[" + sattrs + " ]";

					std::string val = sub->nodeValue();
					if (ignorews)
					{
						size_t lo = 0, hi = val.length();
						while (val[lo] && isspace((unsigned char)val[lo])) lo++;
						while (lo < hi && isspace((unsigned char)val[hi-1])) hi--;
						val = val.substr(lo, hi - lo);
						if (val.empty()) return;
					}

					if (val.length() > 80)
						val = val.substr(0, 77) + "[...]";

					out += ": " + val + "\n";
					fprintf(stderr, "%s", out.c_str());
#ifdef WIN32
					OutputDebugStringA(out.c_str());
#endif
					return;
				}
			}
			if (!subs || subs->length() == 0)
			{
					out += qName(node);
					if (!sattrs.empty())
						out += "[" + sattrs + " ]";
					out += "\n";
					fprintf(stderr, "%s", out.c_str());
#ifdef WIN32
					OutputDebugStringA(out.c_str());
#endif
					return;
			}
			out += "<" + qName(node) + sattrs + ">";
		}
		out += "\n";
		fprintf(stderr, "%s", out.c_str());
#ifdef WIN32
		OutputDebugStringA(out.c_str());
#endif

		Print(subs, ignorews, depth+1);
	}
}
