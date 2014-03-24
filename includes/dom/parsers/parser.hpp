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

#ifndef __DOM_PARSERS_PARSER_HPP__
#define __DOM_PARSERS_PARSER_HPP__

#include <filesystem.hpp>
#include <dom/nodes/document.hpp>

namespace dom { namespace parsers {

	struct Parser
	{
		virtual ~Parser() {};
		virtual bool supportsChunks() const = 0;
		virtual bool onData(const void* data, size_t length) = 0;
		virtual DocumentPtr onFinish() = 0;
	};
	using ParserPtr = std::shared_ptr<Parser>;

	DocumentPtr parseFile(const ParserPtr& parser, const filesystem::path& path);

	static inline DocumentPtr parseDocument(const ParserPtr& parser, const void* data, size_t size)
	{
		if (!parser)
			return nullptr;

		if (!parser->onData(data, size))
			return nullptr;

		return parser->onFinish();
	}
}
}

#endif // __DOM_PARSERS_PARSER_HPP__
