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

#ifndef __DOM_DOCUMENT_HPP__
#define __DOM_DOCUMENT_HPP__

#include <dom/nodes/node.hpp>

namespace dom
{
	struct XmlDocument : XmlNode
	{
		static XmlDocumentPtr create();
		static XmlDocumentPtr fromFile(const char* path);

		virtual XmlElementPtr documentElement() = 0;
		virtual void setDocumentElement(const XmlElementPtr& elem) = 0;
		virtual XmlDocumentFragmentPtr associatedFragment() = 0;
		virtual void setFragment(const XmlDocumentFragmentPtr& fragment) = 0;

		virtual XmlElementPtr createElement(const std::string& tagName) = 0;
		virtual XmlTextPtr createTextNode(const std::string& data) = 0;
		virtual XmlAttributePtr createAttribute(const std::string& name, const std::string& value) = 0;
		virtual XmlDocumentFragmentPtr createDocumentFragment() = 0;

		virtual XmlNodeListPtr getElementsByTagName(const std::string& tagName) = 0;
		virtual XmlElementPtr getElementById(const std::string& elementId) = 0;
	};
}

#endif // __DOM_DOCUMENT_HPP__