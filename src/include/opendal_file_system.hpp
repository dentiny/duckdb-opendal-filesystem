#pragma once

#include "duckdb/common/helper.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/typedefs.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"
#include "opendal_open_options.hpp"

// Forward declaration.
namespace opendal {
class Operator;
class Reader;
} // namespace opendal

namespace duckdb {

// Forward declaration.
class OpenDALFileHandle;

class OpenDALFileSystem {
public:
	// TODO(hjiang): revisit config to represent the configuration of the OpenDAL operator.
	explicit OpenDALFileSystem(const unordered_map<string, string> &config_p = {});

public:
	unique_ptr<OpenDALFileHandle> Open(const string &path_p, OpenDALOpenOptions options_p);
	bool Exists(const string &path_p) const;
	void CopyFile(const string &source_p, const string &target_p);
	void CreateDirectory(const string &path_p);
	vector<string> ListDirectory(const string &path_p) const;
	void MoveFile(const string &source_p, const string &target_p);
	void RemoveFile(const string &path_p);
	bool CanHandleFile(const string &path_p) const;
	// Accept filepath directly.
	bool IsManuallySet() const;

private:
	unordered_map<string, string> config;
};

class OpenDALFileHandle {
public:
	OpenDALFileHandle(unique_ptr<opendal::Operator> op_p, string path_p, OpenDALOpenOptions options_p);
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
