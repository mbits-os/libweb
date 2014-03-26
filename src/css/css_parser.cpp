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
#include <css/parser.hpp>
#include <cctype>
#include <utils.hpp>

namespace css
{
	using It = std::string::const_iterator;

	namespace terminals
	{
		void S(It& it, It end)
		{
			while (it != end && std::isspace((unsigned char)*it))
				++it;
		}

		bool isnmstart(int c)
		{
			return std::isalpha(c) || c == '_' || c == '-';
		}

		bool isnmchar(int c)
		{
			return std::isalnum(c) || c == '_' || c == '-';
		}

		bool IDENT(It& it, It end, std::string& ident)
		{
			auto start = it;
			if (it != end && *it == '-') ++it;

			if (it == end || !isnmstart((unsigned char)*it))
			{
				it = start;
				return false;
			}
			++it;

			while (it != end && isnmchar((unsigned char)*it))
				++it;

			ident.assign(start, it);
			return true;
		}

		bool NUMBER(It& it, It end, std::string& ident)
		{
			auto start = it;
			if (it != end && (*it == '-' || *it == '+')) ++it;

			auto num = it;
			while (it != end && std::isdigit((unsigned char)*it))
				++it;

			if (it != end && *it == '.')
			{
				++it;
				while (it != end && std::isdigit((unsigned char)*it))
					++it;
			}

			if (it == num)
			{
				it = start;
				return false;
			}

			ident.assign(start, it);
			return true;
		}

		bool HASH(It& it, It end, std::string& ident)
		{
			auto start = it;
			if (it == end || *it != '#')
				return false;

			++it;
			if (it == end || !isnmchar((unsigned char)*it))
			{
				it = start;
				return false;
			}

			while (it != end && isnmchar((unsigned char)*it))
				++it;

			ident.assign(start, it);
			return true;
		}

		bool STRING(It& it, It end, std::string& ident)
		{
			auto start = it;
			if (it == end)
				return false;

			auto start_token = *it;
			if (start_token != '\'' && start_token != '"')
			{
				it = start;
				return false;
			}

			++it;

			auto prev = it;
			ident.clear();
			while (it != end)
			{
				if (*it == start_token)
				{
					ident.append(prev, it);
					++it;
					return true;
				}

				if (*it == '\\')
				{
					ident.append(prev, it);
					++it;
					if (it == end)
						return false;

					switch (*it)
					{
					case '\\': ident.push_back('\\'); break;
					case 'a':  ident.push_back('\a'); break;
					case 'b':  ident.push_back('\b'); break;
					case 'r':  ident.push_back('\r'); break;
					case 'n':  ident.push_back('\n'); break;
					case 't':  ident.push_back('\t'); break;
					case '"':  ident.push_back('\"'); break;
					case '\'': ident.push_back('\''); break;
					}
				}

				++it;
			}
				
			it = start;
			return false;
		}

		/*
		lowalpha       = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" |
						 "i" | "j" | "k" | "l" | "m" | "n" | "o" | "p" |
						 "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" |
						 "y" | "z"
		hialpha        = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
						 "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
						 "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
		alpha          = lowalpha | hialpha
		digit          = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
						 "8" | "9"
		safe           = "$" | "-" | "_" | "." | "+"
		extra          = "!" | "*" | "'" | "(" | ")" | ","
		national       = "{" | "}" | "|" | "\" | "^" | "~" | "[" | "]" | "`"
		punctuation    = "<" | ">" | "#" | "%" | <">
		reserved       = ";" | "/" | "?" | ":" | "@" | "&" | "="

		In ASCII, this alphabet happens to be inside <33, 126>
		*/

		bool isurl(int c)
		{
			return c >= 33 && c <= 126;
		}

		bool URL(It& it, It end, std::string& ident)
		{
			auto start = it;
			while (it != end && isurl(*it) && *it != '(' && *it != ')') ++it; // the () are reserved in CSS for the functions

			if (it != start && (it == end || *it == ')'))
			{
				ident.assign(start, it);
				std::trim(ident);
				return true;
			}

			it = start;
			return false;
		}

		// look for end or ;, while maintaining the balance of (), [], {}, "" and ''

		void lookFor(It& it, It end, char warden)
		{
			std::string tmp;

			while (it != end)
			{
				if (*it == warden)
					return;

				switch (*it)
				{
				case '(': lookFor(it, end, ')'); break;
				case '[': lookFor(it, end, ']'); break;
				case '{': lookFor(it, end, '}'); break;
				case '"':
					if (!STRING(it, end, tmp))
						it = end;
					break;
				case '\'':
					if (!STRING(it, end, tmp))
						it = end;
					break;
				}

				if (it != end)
					++it;
			}
		}

		bool RECOVERY(It& it, It end, std::string& ident, char warden)
		{
			auto start = it;
			lookFor(it, end, warden);
			ident.assign(start, it);

			return start != it;
		}
	}

	bool readPropName(It& it, It end, std::string& name)
	{
		terminals::S(it, end);
		auto save = it;
		if (terminals::IDENT(it, end, name))
			return true;

		// wrong characters in the input...
		// try to recover...
		it = save;
		while (it != end && !std::isspace((unsigned char)*it))
			++it;
		name.assign(save, it);
		return false;
	}

	bool readProperty(It& it, It end, Declaration& decl)
	{
		auto save = it;
		while (it != end && *it != ':') ++it;
		decl.failed = !readPropName(save, it, decl.name);

		if (!decl.failed)
		{
			terminals::S(save, it);
			if (save != it)
				decl.failed = true;
		}

		if (it != end)
			++it;

		return true;
	}

	bool readTerm(It& it, It end, Term& term, bool url_allowed);

	bool readFunction(It& it, It end, Term& term, bool url_allowed)
	{
		if (it == end || *it != '(')
			return false;
		++it;
		terminals::S(it, end);

		Term arg;
		while (readTerm(it, end, arg, url_allowed))
			term.arguments.push_back(std::move(arg));

		terminals::S(it, end);
		if (it == end || *it != ')')
			return false;

		++it;
		return true;
	}

	bool readTerm(It& it, It end, Term& term, bool url_allowed)
	{
		if (it == end)
			return false;

		// operator? | IMPORTANT_SYM
		term.op = TERM_OP::NONE;
		if (*it == '/' || *it == ',' || *it == '!')
		{
			switch (*it)
			{
			case '/': term.op = TERM_OP::SLASH; break;
			case ',': term.op = TERM_OP::COMMA; break;
			case '!': term.op = TERM_OP::EXCLAIM; break;
			}
			++it;
			terminals::S(it, end);
		}

		if (term.op == TERM_OP::EXCLAIM)
			return terminals::IDENT(it, end, term.value); // !important

		struct
		{
			bool(*terminal)(It&, It, std::string&);
			TERM_TYPE type;
		} symbols[] = {
			{ terminals::IDENT, TERM_TYPE::IDENT },
			{ terminals::NUMBER, TERM_TYPE::NUMBER },
			{ terminals::HASH, TERM_TYPE::HASH },
			{ terminals::STRING, TERM_TYPE::STRING }
		};

		term.type = TERM_TYPE::UNKNOWN;
		if (url_allowed)
		{
			if (terminals::STRING(it, end, term.value))
				term.type = TERM_TYPE::STRING;
			else  if (terminals::URL(it, end, term.value))
				term.type = TERM_TYPE::URL;
		}

		if (term.type == TERM_TYPE::UNKNOWN) for (auto&& sym : symbols)
		{
			if (sym.terminal(it, end, term.value))
			{
				term.type = sym.type;
				break;
			}
		}

		if (term.type == TERM_TYPE::UNKNOWN)
		{
			if (terminals::RECOVERY(it, end, term.value, url_allowed ? ')' : ';'))
			{
				term.type = TERM_TYPE::FAILURE;
				return true;
			}
			return false;
		}

		if (term.type == TERM_TYPE::IDENT) // function?
		{
			if (it != end && *it == '(')
			{
				term.type = TERM_TYPE::FUNCTION;
				if (std::tolower((const std::string&)term.value) == "url")
				{
					if (!readFunction(it, end, term, true))
						return false;

					if (term.arguments.size() != 1)
						return false;

					auto type = term.arguments.front().type;
					return type == TERM_TYPE::STRING || type == TERM_TYPE::URL;
				}

				if (!readFunction(it, end, term, false))
					return false;
			}
		}
		else if (term.type == TERM_TYPE::NUMBER) // percentage || dimension?
		{
			if (it != end && *it == '%')
			{
				++it;
				term.value.push_back('%');
			}
			else
			{
				std::string tmp;
				if (terminals::IDENT(it, end, tmp))
					term.value.append(tmp);
			}
		}

		terminals::S(it, end);
		return true;
	}

	bool readExpr(It& it, It end, Declaration& decl)
	{
		Term term;
		while (readTerm(it, end, term, false))
		{
			decl.expression.push_back(std::move(term));
			if (term.type == TERM_TYPE::FAILURE)
				decl.failed = true;
		}

		terminals::S(it, end);
		return true;
	}

	bool readDeclaration(It& it, It end, Declaration& decl)
	{
		decl.failed = false;

		terminals::S(it, end);
		if (!readProperty(it, end, decl))
			return false;

		terminals::S(it, end);
		return readExpr(it, end, decl);
	}

	Declarations read_style(const std::string& style)
	{
		Declarations out;
		Declaration decl;
		auto it = style.begin(), end = style.end();
		while (readDeclaration(it, end, decl))
		{
			out.push_back(std::move(decl));
			terminals::S(it, end);
			if (it != end && *it == ';')
				++it;
			else break;
			terminals::S(it, end);
		}

		return out;
	}
}
