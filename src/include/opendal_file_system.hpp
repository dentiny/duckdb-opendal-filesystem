#pragma once

#include "duckdb/common/helper.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/typedefs.hpp"
#include "duckdb/common/unordered_map.hpp"

// Forward declaration.
namespace opendal {
class Operator;
class Reader;
} // namespace opendal

namespace duckdb {

struct OpenDALOpenOptions {
	bool read = true;
	bool write = false;
	bool create = false;
	bool truncate = false;
	bool append = false;

	static OpenDALOpenOptions ReadOnly();
	static OpenDALOpenOptions WriteOnly(bool truncate_p = true);
	static OpenDALOpenOptions ReadWrite(bool create_p = true);
};

// Forward declaration.
class OpenDALFileHandle;

class OpenDALFileSystem {
public:
	explicit OpenDALFileSystem(string scheme_p, const unordered_map<string, string> &config_p = {});

public:
	unique_ptr<OpenDALFileHandle> Open(const string &path_p, OpenDALOpenOptions options_p);
	bool Exists(const string &path_p) const;

private:
	string scheme;
	unordered_map<string, string> config;
};

class OpenDALFileHandle {
public:
	OpenDALFileHandle(unique_ptr<opendal::Operator> op_p, string path_p, OpenDALOpenOptions options_p, bool exists_p);
	~OpenDALFileHandle() noexcept;
	OpenDALFileHandle(const OpenDALFileHandle &) = delete;
	OpenDALFileHandle &operator=(const OpenDALFileHandle &) = delete;

public:
	idx_t Read(void *buffer_p, idx_t size_p);
	idx_t Read(void *buffer_p, idx_t size_p, idx_t offset_p);
	idx_t Write(const void *buffer_p, idx_t size_p);
	idx_t Write(const void *buffer_p, idx_t size_p, idx_t offset_p);
	void Seek(idx_t offset_p);
	idx_t Tell() const;
	idx_t Size() const;
	void Truncate(idx_t size_p);
	void Flush();
	void Close();

private:
	void EnsureOpen() const;
	void EnsureReadable() const;
	void EnsureWritable() const;
	idx_t ReadAt(void *buffer_p, idx_t size_p, idx_t offset_p);
	idx_t WriteAt(const void *buffer_p, idx_t size_p, idx_t offset_p);

private:
	unique_ptr<opendal::Operator> op;
	unique_ptr<opendal::Reader> reader;
	string path;
	OpenDALOpenOptions options;
	string data;
	idx_t position = 0;
	bool dirty = false;
	bool closed = false;
};

} // namespace duckdb
