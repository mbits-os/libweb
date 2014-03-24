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

#ifndef __DOM_DOMFWD_HPP__
#define __DOM_DOMFWD_HPP__

#include <memory>

namespace dom
{
	struct XmlDocument;
	struct XmlNode;
	struct XmlNodeList;
	struct XmlElement;
	struct XmlAttribute;
	struct XmlText;
	struct XmlDocumentFragment;

	using XmlDocumentPtr         = std::shared_ptr<XmlDocument>;
	using XmlNodePtr             = std::shared_ptr<XmlNode>;
	using XmlNodeListPtr         = std::shared_ptr<XmlNodeList>;
	using XmlElementPtr          = std::shared_ptr<XmlElement>;
	using XmlAttributePtr        = std::shared_ptr<XmlAttribute>;
	using XmlTextPtr             = std::shared_ptr<XmlText>;
	using XmlDocumentFragmentPtr = std::shared_ptr<XmlDocumentFragment>;
}

#endif // __DOM_DOMFWD_HPP__