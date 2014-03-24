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
#include <dom/parsers/encoding_db.hpp>

#include <mt.hpp>
#include <utils.hpp>
#include <vector>

namespace dom { namespace parsers {

	struct Encoding
	{
		const char* m_name;
		const int* m_table;
		size_t m_size;
		Encoding(const char* name, const int* table, size_t size)
			: m_name(name)
			, m_table(table)
			, m_size(size)
		{
		}
	};

	class EncodingDB : private mt::AsyncData
	{
		using Encodings = std::vector<Encoding>;

		Encodings m_encodings;
		filesystem::path m_dbPath;
		char* m_data;
		bool m_opened;

		void _close();
		bool _open();

		static EncodingDB& get()
		{
			static EncodingDB db;
			return db;
		}

		const Encoding* _find(std::string enc)
		{
			std::tolower(enc);
			auto it = std::lower_bound(m_encodings.begin(), m_encodings.end(), enc,
				[](const Encoding& enc, const std::string& key)
			{
				// this supposed to be equivalent of "less(enc.m_name, key)" (notice reversed arguments)
				return key.compare(enc.m_name) > 0;
			});
			if (it == m_encodings.end() || enc.compare(it->m_name) != 0)
				return nullptr;
			return &*it;
		}

		EncodingDB()
			: m_data(nullptr)
			, m_opened(false)
		{
		}
	public:
		static mt::AsyncData& guard() { return get(); }
		static void init(const filesystem::path& path) { get().m_dbPath = path; close(); }
		static void close() { get()._close(); }
		static bool open() { return get()._open(); }
		static const Encoding* find(const std::string& enc) { return get()._find(enc); }
	};

	namespace
	{
		struct File
		{
			FILE* m_ptr;
			size_t m_size;
			File(const filesystem::path& path)
			{
				m_ptr = fopen(path.native().c_str(), "rb");
				m_size = (size_t)filesystem::file_size(path);
			}

			~File()
			{
				if (*this)
					fclose(m_ptr);
			}

			operator bool() { return m_ptr != nullptr; }

			size_t read(void* dst, size_t bytes) { return fread(dst, 1, bytes, m_ptr); }
			template <typename T> bool read(T& t) { return read(&t, sizeof(T)) == sizeof(T); }

			size_t size() { return m_size; }
		};

		struct AutoCloser
		{
			char*& data;
			bool close;
			AutoCloser(char*& data) : data(data), close(true) {}
			~AutoCloser()
			{
				if (close)
				{
					delete[] data;
					data = nullptr;
				}
			}
			void release() { close = false; }
		};

		struct FileHeader
		{
			unsigned int magic, count, strings;
		};

		struct TableHeader
		{
			unsigned int string, table, size;
		};
	}

	void EncodingDB::_close()
	{
		if (!m_opened)
			return;

		m_encodings.clear();

		delete[] m_data;
		m_data = nullptr;

		m_opened = false;
	}

	bool EncodingDB::_open()
	{
		if (m_opened)
			return true;

		File file(m_dbPath);
		if (!file)
			return false;

		FileHeader header;
		if (!file.read(header)) return false;

		static const unsigned int MAGIC = 0x54455343;
		if (header.magic != MAGIC) return false;
		size_t stringsStart = sizeof(header)+sizeof(TableHeader)*header.count;
		size_t minimalOffset = stringsStart + header.strings;
		minimalOffset = ((minimalOffset + 3) >> 2) << 2;
		if (minimalOffset > file.size())
			return false;

		size_t buffer = file.size() - stringsStart;
		m_data = new (std::nothrow) char[buffer];
		if (!m_data)
			return false;
		AutoCloser guard(m_data);

		m_encodings.reserve(header.count);
		for (unsigned int i = 0; i < header.count; ++i)
		{
			TableHeader head;
			if (!file.read(head)) return false;

			if (head.table < minimalOffset)
				return false;
			if (head.table + head.size > file.size())
				return false;
			if (head.string < stringsStart || head.string >= (stringsStart + header.strings))
				return false;

			const char* name = m_data + head.string - stringsStart;
			const int* table = (int*)(m_data + head.table - stringsStart);

			m_encodings.emplace_back(name, table, head.size);
		}

		if (file.read(m_data, buffer) != buffer)
			return false;

		guard.release();
		return m_opened = true;
	}

	void init(const filesystem::path& path)
	{
		Synchronize on(EncodingDB::guard());

		EncodingDB::init(path);
	}

	void reload(const filesystem::path& path)
	{
		Synchronize on(EncodingDB::guard());

		EncodingDB::init(path); // re-init.
	}

	bool loadCharset(const std::string& encoding, int(&table)[256])
	{
		Synchronize on(EncodingDB::guard());

		if (!EncodingDB::open())
			return false;

		auto obj = EncodingDB::find(encoding);

		if (!obj)
			return false;

		if (obj->m_size < sizeof(table))
			return false;

		memcpy(table, obj->m_table, sizeof(table));
		return true;
	}
}}
