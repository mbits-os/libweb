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

#ifndef __DOM_NODE_HPP__
#define __DOM_NODE_HPP__

#include <string>
#include <iostream>
#include <dom/domfwd.hpp>

namespace dom
{
	struct NSData
	{
		const char* key;
		const char* ns;
	};

	typedef NSData* Namespaces;

	struct QName
	{
		std::string nsName;
		std::string localName;

		bool operator == (const QName& right) const
		{
			return nsName == right.nsName && localName == right.localName;
		}
		bool operator != (const QName& right) const
		{
			return !(*this == right);
		}
	};

	inline std::ostream& operator << (std::ostream& o, const QName& qname)
	{
		if (!qname.nsName.empty())
			o << "{" << qname.nsName << "}";
		return o << qname.localName;
	}

	enum NODE_TYPE
	{
		DOCUMENT_NODE  = 0,
		ELEMENT_NODE   = 1,
		ATTRIBUTE_NODE = 2,
		TEXT_NODE      = 3,
		DOCUMENT_FRAGMENT_NODE = 4
	};

	struct XmlNode
	{
		virtual ~XmlNode() {}
		virtual std::string nodeName() const = 0;
		virtual const QName& nodeQName() const = 0;
		virtual std::string nodeValue() const = 0;
		virtual std::string stringValue() { return nodeValue(); } // nodeValue for TEXT, ATTRIBUTE, and - coincidently - DOCUMENT; innerText for ELEMENT; used in xpath
		virtual void nodeValue(const std::string& val) = 0;
		virtual NODE_TYPE nodeType() const = 0;

		virtual XmlNodePtr parentNode() = 0;
		virtual XmlNodeListPtr childNodes() = 0;
		virtual XmlNodePtr firstChild() = 0;
		virtual XmlNodePtr lastChild() = 0;
		virtual XmlNodePtr previousSibling() = 0;
		virtual XmlNodePtr nextSibling() = 0;

		virtual XmlDocumentPtr ownerDocument() = 0;
		virtual bool insertBefore(const XmlNodePtr& child, const XmlNodePtr& before = nullptr) = 0;
		virtual bool insertBefore(const XmlNodeListPtr& children, const XmlNodePtr& before = nullptr) = 0;
		virtual bool appendChild(const XmlNodePtr& newChild) = 0;
		virtual bool replaceChild(const XmlNodePtr& newChild, const XmlNodePtr& oldChild) = 0;
		virtual bool replaceChild(const XmlNodeListPtr& newChildren, const XmlNodePtr& oldChild) = 0;
		virtual bool removeChild(const XmlNodePtr& child) = 0;

		virtual void* internalData() = 0;

		virtual XmlNodePtr find(const std::string& path, const Namespaces& ns) = 0;
		virtual XmlNodePtr find(const std::string& path) { return find(path, nullptr); }
		virtual XmlNodeListPtr findall(const std::string& path, const Namespaces& ns) = 0;
	};

	struct XmlChildNode : XmlNode
	{
		virtual bool before(const XmlNodePtr& node) = 0;
		virtual bool before(const std::string& data) = 0;
		virtual bool before(const XmlNodeListPtr& nodes) = 0;
		virtual bool after(const XmlNodePtr& node) = 0;
		virtual bool after(const std::string& data) = 0;
		virtual bool after(const XmlNodeListPtr& nodes) = 0;
		virtual bool replace(const XmlNodePtr& node) = 0;
		virtual bool replace(const std::string& data) = 0;
		virtual bool replace(const XmlNodeListPtr& nodes) = 0;
		virtual bool remove() = 0;
	};

	struct XmlParentNode : XmlChildNode
	{
		virtual bool prepend(const XmlNodePtr& node) = 0;
		virtual bool prepend(const XmlNodeListPtr& nodes) = 0;
		virtual bool prepend(const std::string& data) = 0;
		virtual bool append(const XmlNodePtr& node) = 0;
		virtual bool append(const XmlNodeListPtr& nodes) = 0;
		virtual bool append(const std::string& data) = 0;
	};
}

#endif // __DOM_NODE_HPP__