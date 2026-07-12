#pragma once

#include "duckdb/common/file_system.hpp"
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

class OpenDALFileSystem : public FileSystem {
public:
	// TODO(hjiang): revisit config to represent the configuration of the OpenDAL operator.
	explicit OpenDALFileSystem(const unordered_map<string, string> &config_p = {});

public:
	unique_ptr<FileHandle> OpenFile(const string &path_p, FileOpenFlags flags_p,
	                                optional_ptr<FileOpener> opener_p = nullptr) override;
	void Read(FileHandle &handle_p, void *buffer_p, int64_t size_p, idx_t offset_p) override;
	void Write(FileHandle &handle_p, void *buffer_p, int64_t size_p, idx_t offset_p) override;
	int64_t Read(FileHandle &handle_p, void *buffer_p, int64_t size_p) override;
	int64_t Write(FileHandle &handle_p, void *buffer_p, int64_t size_p) override;
	bool Trim(FileHandle &handle_p, idx_t offset_p, idx_t length_p) override;
	int64_t GetFileSize(FileHandle &handle_p) override;
	timestamp_t GetLastModifiedTime(FileHandle &handle_p) override;
	string GetVersionTag(FileHandle &handle_p) override;
	FileType GetFileType(FileHandle &handle_p) override;
	FileMetadata Stats(FileHandle &handle_p) override;
	void FileSync(FileHandle &handle_p) override;
	void Reset(FileHandle &handle_p) override;
	void Seek(FileHandle &handle_p, idx_t location_p) override;
	idx_t SeekPosition(FileHandle &handle_p) override;
	void Truncate(FileHandle &handle_p, int64_t size_p) override;
	void CreateDirectory(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	void CreateDirectoriesRecursive(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	bool DirectoryExists(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	bool ListFiles(const string &path_p, const std::function<void(const string &, bool)> &callback_p,
	               FileOpener *opener_p = nullptr) override;
	void MoveFile(const string &source_p, const string &target_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	void RemoveDirectory(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	void RemoveFile(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	void RemoveFiles(const vector<string> &paths_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	bool TryRemoveFile(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	bool FileExists(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	bool IsPipe(const string &path_p, optional_ptr<FileOpener> opener_p = nullptr) override;
	bool CanHandleFile(const string &path_p) override;
	bool CanSeek() override;
	bool IsLocalFileSystem() const override;
	string GetName() const override;
	string GetHomeDirectory() override;
	string ExpandPath(const string &path_p) override;
	string PathSeparator(const string &path_p) override;
	bool IsPathAbsolute(const string &path_p) override;
	vector<OpenFileInfo> Glob(const string &path_p, FileOpener *opener_p = nullptr) override;
	bool OnDiskFile(FileHandle &handle_p) override;
	// Accept filepath directly.
	bool IsManuallySet() override;

private:
	unordered_map<string, string> config;
};

class OpenDALFileHandle : public FileHandle {
	friend class OpenDALFileSystem;

public:
	OpenDALFileHandle(OpenDALFileSystem &fs_p, unique_ptr<opendal::Operator> op_p, string full_path_p, string path_p,
	                  FileOpenFlags flags_p, OpenDALOpenOptions options_p);
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
	void Close() override;

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
