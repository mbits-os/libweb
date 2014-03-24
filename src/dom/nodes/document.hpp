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

#ifndef __DOM_INTERNAL_DOCUMENT_HPP__
#define __DOM_INTERNAL_DOCUMENT_HPP__

#include <dom/nodes/document.hpp>

namespace dom { namespace impl {

	class Document : public dom::XmlDocument, public std::enable_shared_from_this<Document>
	{
		QName m_qname;
		dom::XmlElementPtr root;
		dom::XmlDocumentFragmentPtr fragment;
	public:
		Document();

		std::string nodeName() const override { return m_qname.localName; }
		const QName& nodeQName() const override { return m_qname; }
		std::string nodeValue() const override { return std::string(); }
		void nodeValue(const std::string&) override {}
		NODE_TYPE nodeType() const override { return DOCUMENT_NODE; }

		XmlNodePtr parentNode() override { return nullptr; }
		XmlNodePtr firstChild() override { return documentElement(); }
		XmlNodePtr lastChild() override { return documentElement(); }
		XmlNodePtr previousSibling() override { return nullptr; }
		XmlNodePtr nextSibling() override { return nullptr; }
		XmlNodeListPtr childNodes() override;
		XmlDocumentPtr ownerDocument() override { return shared_from_this(); }
		bool insertBefore(const XmlNodePtr& child, const XmlNodePtr& before = nullptr) override { return false; }
		bool insertBefore(const XmlNodeListPtr& children, const XmlNodePtr& before = nullptr) override { return false; }
		bool appendChild(const XmlNodePtr& newChild) override { return false; }
		bool replaceChild(const XmlNodePtr& newChild, const XmlNodePtr& oldChild) override { return false; }
		bool replaceChild(const XmlNodeListPtr& newChildren, const XmlNodePtr& oldChild) override { return false; }
		bool removeChild(const XmlNodePtr& child) override { return false; }
		void* internalData() override { return nullptr; }

		dom::XmlElementPtr documentElement() override { return root; }
		void setDocumentElement(const dom::XmlElementPtr& elem) override;
		dom::XmlDocumentFragmentPtr associatedFragment() override { return fragment; }
		void setFragment(const XmlDocumentFragmentPtr& f) override;
		dom::XmlElementPtr createElement(const std::string& tagName) override;
		dom::XmlTextPtr createTextNode(const std::string& data) override;
		dom::XmlAttributePtr createAttribute(const std::string& name, const std::string& value) override;
		dom::XmlDocumentFragmentPtr createDocumentFragment() override;
		dom::XmlNodeListPtr getElementsByTagName(const std::string& tagName) override;
		dom::XmlElementPtr getElementById(const std::string& elementId) override;
		XmlNodePtr find(const std::string& path, const Namespaces& ns) override;
		XmlNodeListPtr findall(const std::string& path, const Namespaces& ns) override;
	};
}}

#endif // __DOM_INTERNAL_DOCUMENT_HPP__
