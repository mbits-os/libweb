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

#ifndef __DOM_XPATH_HPP__
#define __DOM_XPATH_HPP__

namespace dom
{
	namespace xpath
	{
		enum AXIS
		{
			// forward
			AXIS_CHILD,
			AXIS_DESCENDANT,
			AXIS_ATTRIBUTE,
			AXIS_SELF,
			AXIS_DESCENDANT_OR_SELF,
			// reverse
			AXIS_PARENT,
			AXIS_ANCESTOR,
			AXIS_ANCESTOR_OR_SELF,
		};

		enum TEST
		{
			// kind test
			TEST_NODE,         // 'node()'
			TEST_TEXT,         // 'text()'
			TEST_ELEMENT,      // 'element()' or 'element(ns:loc)' or 'ns:loc'
			TEST_ATTRIBUTE,    // 'attribute()' or 'attribute(ns:loc)' or '@ns:loc'
			TEST_DOCUMENT_NODE // 'document-node()' or '</' regexp
		};

		enum PRED
		{
			PRED_EXISTS,
			PRED_EQUALS
		};

		struct SimpleSelector
		{
			AXIS m_axis;
			TEST m_test;
			QName m_name;
			SimpleSelector(): m_axis(AXIS_CHILD), m_test(TEST_NODE) {}
			bool passable(NodePtr& node);
			void select(const NodePtr& context, std::list<NodePtr>& list);
		private:
			void test(const NodePtr& node, std::list<NodePtr>& list);
			void select(const NodeListPtr& nodes, std::list<NodePtr>& list);

			void child(const NodePtr& context, std::list<NodePtr>& list);
			void descendant(const NodePtr& context, std::list<NodePtr>& list);
			void attribute(const NodePtr& context, std::list<NodePtr>& list);
			void self(const NodePtr& context, std::list<NodePtr>& list);
			void descendant_or_self(const NodePtr& context, std::list<NodePtr>& list);
			void parent(const NodePtr& context, std::list<NodePtr>& list);
			void ancestor(const NodePtr& context, std::list<NodePtr>& list);
			void ancestor_or_self(const NodePtr& context, std::list<NodePtr>& list);
		};
		typedef std::list<SimpleSelector> SimpleSelectors;

		struct Predicate
		{
			PRED m_type;
			SimpleSelectors m_selectors;
			std::string m_value;
			Predicate(): m_type(PRED_EXISTS) {}
			bool test(const NodePtr& context);
		};
		typedef std::list<Predicate> Predicates;

		struct Segment
		{
			SimpleSelector m_selector;
			Predicates m_preds;
			void select(const NodePtr& context, std::list<NodePtr>& list);
		};
		typedef std::list<Segment> Segments;

		struct XPath
		{
			XPath(const std::string& xpath, const Namespaces& ns);
			NodePtr find(const NodePtr& context);
			NodeListPtr findall(const NodePtr& context);
			Segments m_segments;
		private:
			const char* readSegment(const char* ptr, const char* end, Segment& seg, const Namespaces& ns);
		};
		std::ostream& operator << (std::ostream& o, const XPath& qname);
	};
}

#endif // __DOM_XPATH_HPP__
