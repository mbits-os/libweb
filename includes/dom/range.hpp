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

#ifndef __DOM_RANGE_HPP__
#define __DOM_RANGE_HPP__

#include <iterator>
#include <dom/nodes/node.hpp>

namespace dom
{
	namespace range
	{
		template <typename NodeType>
		struct NodeSelector;

		template <>
		struct NodeSelector<XmlNode>
		{
			using node_t = XmlNode;
			using node_ptr = std::shared_ptr<node_t>;

			static node_ptr item(const XmlNodeListPtr& list, size_t index) { return list->item(index); }
		};

		template <>
		struct NodeSelector<XmlElement>
		{
			using node_t = XmlElement;
			using node_ptr = std::shared_ptr<node_t>;

			static node_ptr item(const XmlNodeListPtr& list, size_t index) { return list->element(index); }
		};

		template <>
		struct NodeSelector<XmlText>
		{
			using node_t = XmlText;
			using node_ptr = std::shared_ptr<node_t>;

			static node_ptr item(const XmlNodeListPtr& list, size_t index) { return list->text(index); }
		};

		template <>
		struct NodeSelector<XmlAttribute>
		{
			using node_t = XmlAttribute;
			using node_ptr = std::shared_ptr<node_t>;

			static node_ptr item(const XmlNodeListPtr& list, size_t index) { return list->attr(index); }
		};

		template <typename NodeType>
		struct NodeListRangeAdapter : NodeSelector<NodeType>
		{
			using selector_t = NodeSelector<NodeType>;
			using node_t = typename selector_t::node_t;
			using node_ptr = typename selector_t::node_ptr;

			class list_iterator
				: public std::iterator< std::bidirectional_iterator_tag, node_ptr >
			{
			public:
				typedef node_ptr pointer;
				typedef node_ptr reference;

				list_iterator(const XmlNodeListPtr& list, size_t offset) : m_list(list), m_offset(offset) {}

				list_iterator() = default;
				list_iterator(const list_iterator&) = default;
				list_iterator& operator=(const list_iterator&) = default;
				list_iterator(list_iterator&& oth) = default;
				list_iterator& operator=(list_iterator&& oth) = default;

				reference operator*() const
				{
					return selector_t::item(m_list, m_offset);
				}

				pointer operator->() const
				{
					return (std::pointer_traits<pointer>::pointer_to(**this));
				}

				list_iterator& operator++()
				{
					++m_offset;
					return *this;
				}

				list_iterator operator++(int)
				{	// postincrement
					list_iterator _Tmp = *this;
					++*this;
					return (_Tmp);
				}

				list_iterator& operator--()
				{
					++m_offset;
					return *this;
				}

				list_iterator operator--(int)
				{	// postdecrement
					list_iterator _Tmp = *this;
					--*this;
					return (_Tmp);
				}

				bool operator==(const list_iterator& _Right) const
				{	// test for iterator equality
					return (m_list == _Right.m_list && m_offset == _Right.m_offset);
				}

				bool operator!=(const list_iterator& _Right) const
				{	// test for iterator inequality
					return (!(*this == _Right));
				}
			private:
				XmlNodeListPtr m_list;
				size_t m_offset = 0;
			};

			using iterator = list_iterator;
			using const_iterator = list_iterator;

			XmlNodeListPtr list;
			NodeListRangeAdapter(const XmlNodeListPtr& list) : list(list) {}
			iterator begin() const { return iterator(list, 0); }
			iterator end() const
			{
				if (!list)
					return iterator(list, 0);
				return iterator(list, list->length());
			}
		};
	}

	inline range::NodeListRangeAdapter<XmlNode> list_nodes(const XmlNodeListPtr& list)
	{
		return range::NodeListRangeAdapter<XmlNode>(list);
	}

	inline range::NodeListRangeAdapter<XmlElement> list_elements(const XmlNodeListPtr& list)
	{
		return range::NodeListRangeAdapter<XmlElement>(list);
	}

	inline range::NodeListRangeAdapter<XmlText> list_texts(const XmlNodeListPtr& list)
	{
		return range::NodeListRangeAdapter<XmlText>(list);
	}

	inline range::NodeListRangeAdapter<XmlAttribute> list_atts(const XmlNodeListPtr& list)
	{
		return range::NodeListRangeAdapter<XmlAttribute>(list);
	}
}

#endif // __DOM_RANGE_HPP__