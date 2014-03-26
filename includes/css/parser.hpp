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

#ifndef __CSS_PARSER_HPP__
#define __CSS_PARSER_HPP__

#include <filesystem.hpp>
#include <string>
#include <vector>

namespace dom { namespace parsers { struct OutStream; } }

namespace css {
	enum class TERM_OP
	{
		NONE,
		SLASH,
		COMMA,
		EXCLAIM
	};

	enum class TERM_TYPE
	{
		UNKNOWN,
		FAILURE,
		NUMBER, // NUMBER | PERCENTAGE | DIMENSION
		IDENT,
		STRING,
		URL,
		HASH,
		FUNCTION // URI | function
	};

	struct Term;
	using Terms = std::vector<Term>;

	struct Term
	{
		TERM_OP op;
		TERM_TYPE type;
		std::string value;
		Terms arguments; // URI | function
	};

	struct Declaration
	{
		std::string name;
		bool failed;
		Terms expression;
	};

	using Declarations = std::vector<Declaration>;

	Declarations read_style(const std::string& style);

	void serialize_style(dom::parsers::OutStream& stream, const Declarations& ruleset, const std::string& sep);
}

#endif // __CSS_PARSER_HPP__
