#include "opendal_file_system.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/file_opener.hpp"
#include "duckdb/common/limits.hpp"
#include "duckdb/common/types/timestamp.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include "opendal_path.hpp"

#include <opendal.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

namespace duckdb {
namespace {

unique_ptr<opendal::Operator> CreateOperator(const OpenDALPath &path_p, const unordered_map<string, string> &config_p) {
	auto config = config_p;
	if (!path_p.endpoint.empty()) {
		config["endpoint"] = path_p.endpoint;
	}
	if (!path_p.bucket.empty()) {
		config["bucket"] = path_p.bucket;
	}
	return make_uniq<opendal::Operator>(path_p.scheme, config);
}

} // namespace

OpenDALFileSystem::OpenDALFileSystem(const unordered_map<string, string> &config_p) : config(config_p) {
}

unordered_map<string, string> OpenDALFileSystem::ResolveConfig(const OpenDALPath &path_p, const string &full_path_p,
                                                               optional_ptr<FileOpener> opener_p) const {
	auto result = config;
	auto secret_manager = FileOpener::TryGetSecretManager(opener_p);
	auto transaction = FileOpener::TryGetCatalogTransaction(opener_p);
	if (!secret_manager || !transaction) {
		return result;
	}

	auto service = path_p.scheme;
	std::replace(service.begin(), service.end(), '-', '_');
	const auto secret_type = service == "http" ? string("http") : string("opendal_") + service;
	auto secret_match = secret_manager->LookupSecret(*transaction, full_path_p, secret_type);
	if (!secret_match.HasMatch()) {
		return result;
	}
	const auto &secret = dynamic_cast<const KeyValueSecret &>(*secret_match.secret_entry->secret);
	for (const auto &entry : secret.secret_map) {
		auto key = entry.first;
		if (key == "bearer_token") {
			key = "token";
		}
		result[key] = entry.second.ToString();
	}
	return result;
}

unique_ptr<FileHandle> OpenDALFileSystem::OpenFile(const string &path_p, FileOpenFlags flags_p,
                                                   optional_ptr<FileOpener> opener_p) {
	OpenDALOpenOptions options(flags_p);
	if (path_p.empty()) {
		throw InvalidInputException("OpenDAL path must not be empty");
	}
	if (!options.IsValid()) {
		throw InvalidInputException("Invalid OpenDAL file open options");
	}

	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_p);
	}
	if (path.path.empty()) {
		throw InvalidInputException("OpenDAL object path must not be empty");
	}

	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	const bool exists = op->Exists(path.path);
	if (!exists && !options.create) {
		if (flags_p.ReturnNullIfNotExists()) {
			return nullptr;
		}
		throw IOException("OpenDAL object does not exist: %s", path_p);
	}
	return make_uniq<OpenDALFileHandle>(*this, std::move(op), path_p, std::move(path.path), flags_p, options);
}

bool OpenDALFileSystem::FileExists(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path) || path.path.empty()) {
		return false;
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	return op->Exists(path.path);
}

int64_t OpenDALFileSystem::GetFileSize(FileHandle &handle_p) {
	return handle_p.Cast<OpenDALFileHandle>().Size();
}

void OpenDALFileSystem::Read(FileHandle &handle_p, void *buffer_p, int64_t size_p, idx_t offset_p) {
	if (size_p < 0 || handle_p.Cast<OpenDALFileHandle>().Read(buffer_p, static_cast<idx_t>(size_p), offset_p) !=
	                      static_cast<idx_t>(size_p)) {
		throw IOException("Could not read the requested number of bytes from OpenDAL");
	}
}

void OpenDALFileSystem::Write(FileHandle &handle_p, void *buffer_p, int64_t size_p, idx_t offset_p) {
	if (size_p < 0 || handle_p.Cast<OpenDALFileHandle>().Write(buffer_p, static_cast<idx_t>(size_p), offset_p) !=
	                      static_cast<idx_t>(size_p)) {
		throw IOException("Could not write the requested number of bytes to OpenDAL");
	}
}

int64_t OpenDALFileSystem::Read(FileHandle &handle_p, void *buffer_p, int64_t size_p) {
	if (size_p < 0) {
		throw InvalidInputException("OpenDAL read size must not be negative");
	}
	return handle_p.Cast<OpenDALFileHandle>().Read(buffer_p, static_cast<idx_t>(size_p));
}

int64_t OpenDALFileSystem::Write(FileHandle &handle_p, void *buffer_p, int64_t size_p) {
	if (size_p < 0) {
		throw InvalidInputException("OpenDAL write size must not be negative");
	}
	return handle_p.Cast<OpenDALFileHandle>().Write(buffer_p, static_cast<idx_t>(size_p));
}

bool OpenDALFileSystem::Trim(FileHandle &handle_p, idx_t offset_p, idx_t length_p) {
	return false;
}

FileMetadata OpenDALFileSystem::Stats(FileHandle &handle_p) {
	auto &handle = handle_p.Cast<OpenDALFileHandle>();
	if (handle.options.write) {
		FileMetadata result;
		result.file_size = static_cast<int64_t>(handle.Size());
		result.file_type = FileType::FILE_TYPE_REGULAR;
		return result;
	}
	const auto metadata = handle.op->Stat(handle.path);
	FileMetadata result;
	result.file_size = static_cast<int64_t>(metadata.ContentLength());
	result.file_type = metadata.IsFile()  ? FileType::FILE_TYPE_REGULAR
	                   : metadata.IsDir() ? FileType::FILE_TYPE_DIR
	                                      : FileType::FILE_TYPE_INVALID;
	if (metadata.LastModified()) {
		const auto micros =
		    std::chrono::duration_cast<std::chrono::microseconds>(metadata.LastModified()->time_since_epoch()).count();
		result.last_modification_time = Timestamp::FromEpochMicroSeconds(micros);
	}
	return result;
}

timestamp_t OpenDALFileSystem::GetLastModifiedTime(FileHandle &handle_p) {
	return Stats(handle_p).last_modification_time;
}

string OpenDALFileSystem::GetVersionTag(FileHandle &handle_p) {
	auto &handle = handle_p.Cast<OpenDALFileHandle>();
	if (handle.options.write) {
		return string();
	}
	const auto metadata = handle.op->Stat(handle.path);
	return metadata.Etag().value_or(string());
}

FileType OpenDALFileSystem::GetFileType(FileHandle &handle_p) {
	return Stats(handle_p).file_type;
}

void OpenDALFileSystem::FileSync(FileHandle &handle_p) {
	handle_p.Cast<OpenDALFileHandle>().Flush();
}

void OpenDALFileSystem::Reset(FileHandle &handle_p) {
	handle_p.Cast<OpenDALFileHandle>().Seek(0);
}

void OpenDALFileSystem::Seek(FileHandle &handle_p, idx_t location_p) {
	handle_p.Cast<OpenDALFileHandle>().Seek(location_p);
}

idx_t OpenDALFileSystem::SeekPosition(FileHandle &handle_p) {
	return handle_p.Cast<OpenDALFileHandle>().Tell();
}

void OpenDALFileSystem::Truncate(FileHandle &handle_p, int64_t size_p) {
	if (size_p < 0) {
		throw InvalidInputException("OpenDAL truncate size must not be negative");
	}
	handle_p.Cast<OpenDALFileHandle>().Truncate(static_cast<idx_t>(size_p));
}

void OpenDALFileSystem::CreateDirectory(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_p);
	}
	if (path.path.empty()) {
		throw InvalidInputException("OpenDAL directory path must not be empty");
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	op->CreateDir(path.path);
}

void OpenDALFileSystem::CreateDirectoriesRecursive(const string &path_p, optional_ptr<FileOpener> opener_p) {
	CreateDirectory(path_p, opener_p);
}

bool OpenDALFileSystem::DirectoryExists(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path) || path.path.empty()) {
		return false;
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	return op->Stat(path.path).IsDir();
}

bool OpenDALFileSystem::ListFiles(const string &path_p, const std::function<void(const string &, bool)> &callback_p,
                                  FileOpener *opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_p);
	}
	if (path.path.empty()) {
		throw InvalidInputException("OpenDAL directory path must not be empty");
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	auto entries = op->List(path.path);
	for (const auto &entry : entries) {
		callback_p(entry.path, !entry.path.empty() && entry.path.back() == '/');
	}
	return true;
}

void OpenDALFileSystem::MoveFile(const string &source_p, const string &target_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath source;
	OpenDALPath target;
	if (!OpenDALPath::TryParse(source_p, source)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", source_p);
	}
	if (!OpenDALPath::TryParse(target_p, target)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", target_p);
	}
	if (source.path.empty() || target.path.empty()) {
		throw InvalidInputException("OpenDAL move paths must not be empty");
	}

	// Fast path: same scheme.
	if (source.scheme == target.scheme) {
		auto op = CreateOperator(source, ResolveConfig(source, source_p, opener_p));
		op->Rename(source.path, target.path);
		return;
	}

	// Fallback to read and write.
	// TODO(hjiang): parallel read and write.
	auto source_op = CreateOperator(source, ResolveConfig(source, source_p, opener_p));
	auto target_op = CreateOperator(target, ResolveConfig(target, target_p, opener_p));
	auto data = source_op->Read(source.path);
	target_op->Write(target.path, data);
	source_op->Remove(source.path);
}

void OpenDALFileSystem::RemoveDirectory(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_p);
	}
	if (path.path.empty()) {
		throw InvalidInputException("OpenDAL directory path must not be empty");
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	op->Remove(path.path);
}

void OpenDALFileSystem::RemoveFile(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_p);
	}
	if (path.path.empty()) {
		throw InvalidInputException("OpenDAL object path must not be empty");
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	op->Remove(path.path);
}

// TODO(hjiang): investigate batch deletion API.
void OpenDALFileSystem::RemoveFiles(const vector<string> &paths_p, optional_ptr<FileOpener> opener_p) {
	vector<OpenDALPath> paths;
	paths.reserve(paths_p.size());
	for (const auto &path_string : paths_p) {
		OpenDALPath path;
		if (!OpenDALPath::TryParse(path_string, path)) {
			throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_string);
		}
		if (path.path.empty()) {
			throw InvalidInputException("OpenDAL object path must not be empty");
		}
		paths.emplace_back(std::move(path));
	}
	for (idx_t index = 0; index < paths.size(); index++) {
		const auto &path = paths[index];
		auto op = CreateOperator(path, ResolveConfig(path, paths_p[index], opener_p));
		op->Remove(path.path);
	}
}

bool OpenDALFileSystem::TryRemoveFile(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath path;
	if (!OpenDALPath::TryParse(path_p, path) || path.path.empty()) {
		return false;
	}
	auto op = CreateOperator(path, ResolveConfig(path, path_p, opener_p));
	op->Remove(path.path);
	return true;
}

bool OpenDALFileSystem::IsPipe(const string &path_p, optional_ptr<FileOpener> opener_p) {
	return false;
}

bool OpenDALFileSystem::CanHandleFile(const string &path_p) {
	OpenDALPath path;
	return OpenDALPath::TryParse(path_p, path);
}

bool OpenDALFileSystem::CanSeek() {
	return true;
}

bool OpenDALFileSystem::IsLocalFileSystem() const {
	return false;
}

string OpenDALFileSystem::GetName() const {
	return "duckdb_opendalfs";
}

string OpenDALFileSystem::GetHomeDirectory() {
	return string();
}

string OpenDALFileSystem::ExpandPath(const string &path_p) {
	return path_p;
}

string OpenDALFileSystem::PathSeparator(const string &path_p) {
	return "/";
}

bool OpenDALFileSystem::IsPathAbsolute(const string &path_p) {
	OpenDALPath path;
	return OpenDALPath::TryParse(path_p, path);
}

vector<OpenFileInfo> OpenDALFileSystem::Glob(const string &path_p, FileOpener *opener_p) {
	OpenDALPath path;
	const bool is_http = OpenDALPath::TryParse(path_p, path) && path.scheme == "http";
	const bool has_glob =
	    is_http ? path_p.find('*') != string::npos || path_p.find('[') != string::npos : HasGlob(path_p);
	if (has_glob) {
		throw InvalidInputException("Globs (`*`) for generic OpenDAL files are not supported");
	}
	return {OpenFileInfo(path_p)};
}

bool OpenDALFileSystem::OnDiskFile(FileHandle &handle_p) {
	return false;
}

bool OpenDALFileSystem::IsManuallySet() {
	return true;
}

OpenDALFileHandle::OpenDALFileHandle(OpenDALFileSystem &fs_p, unique_ptr<opendal::Operator> op_p, string full_path_p,
                                     string path_p, FileOpenFlags flags_p, OpenDALOpenOptions options_p)
    : FileHandle(fs_p, std::move(full_path_p), flags_p), op(std::move(op_p)), path(std::move(path_p)),
      options(options_p) {
	if (options.write) {
		dirty = true;
	} else {
		reader = make_uniq<opendal::Reader>(op->GetReader(path));
	}
}

OpenDALFileHandle::~OpenDALFileHandle() noexcept {
	Close();
}

void OpenDALFileHandle::EnsureOpen() const {
	if (closed) {
		throw IOException("OpenDAL file handle is closed");
	}
}

void OpenDALFileHandle::EnsureReadable() const {
	EnsureOpen();
	if (!options.read) {
		throw IOException("OpenDAL file handle is not readable");
	}
}

void OpenDALFileHandle::EnsureWritable() const {
	EnsureOpen();
	if (!options.write) {
		throw IOException("OpenDAL file handle is not writable");
	}
}

idx_t OpenDALFileHandle::ReadAt(void *buffer_p, idx_t size_p, idx_t offset_p) {
	if (size_p == 0) {
		return 0;
	}
	if (!buffer_p) {
		throw InvalidInputException("read buffer must not be null");
	}

	if (options.write) {
		if (offset_p >= data.size()) {
			return 0;
		}
		const auto read_size = std::min<idx_t>(size_p, data.size() - offset_p);
		std::memcpy(buffer_p, data.data() + offset_p, read_size);
		return read_size;
	}

	if (offset_p > static_cast<idx_t>(NumericLimits<std::streamoff>::Maximum())) {
		throw IOException("Read offset exceeds OpenDAL reader range");
	}
	if (size_p > static_cast<idx_t>(NumericLimits<std::streamsize>::Maximum())) {
		throw IOException("Read size exceeds OpenDAL reader range");
	}
	reader->Seek(static_cast<std::streamoff>(offset_p), std::ios_base::beg);
	const auto read_size = reader->Read(buffer_p, static_cast<std::streamsize>(size_p));
	return read_size > 0 ? static_cast<idx_t>(read_size) : 0;
}

idx_t OpenDALFileHandle::Read(void *buffer_p, idx_t size_p) {
	EnsureReadable();
	const auto read_size = ReadAt(buffer_p, size_p, position);
	position += read_size;
	return read_size;
}

idx_t OpenDALFileHandle::Read(void *buffer_p, idx_t size_p, idx_t offset_p) {
	EnsureReadable();
	return ReadAt(buffer_p, size_p, offset_p);
}

idx_t OpenDALFileHandle::WriteAt(const void *buffer_p, idx_t size_p, idx_t offset_p) {
	if (size_p == 0) {
		return 0;
	}
	if (!buffer_p) {
		throw InvalidInputException("write buffer must not be null");
	}
	if (size_p > NumericLimits<std::size_t>::Maximum() ||
	    offset_p > static_cast<idx_t>(NumericLimits<std::size_t>::Maximum()) - size_p) {
		throw IOException("Write exceeds addressable object size");
	}
	const auto end = static_cast<std::size_t>(offset_p + size_p);
	if (end > data.size()) {
		data.resize(end, '\0');
	}
	std::memcpy(data.data() + offset_p, buffer_p, size_p);
	dirty = true;
	return size_p;
}

idx_t OpenDALFileHandle::Write(const void *buffer_p, idx_t size_p) {
	EnsureWritable();
	const auto offset = position;
	const auto write_size = WriteAt(buffer_p, size_p, offset);
	position = offset + write_size;
	return write_size;
}

idx_t OpenDALFileHandle::Write(const void *buffer_p, idx_t size_p, idx_t offset_p) {
	EnsureWritable();
	if (offset_p != position) {
		throw IOException("Non-sequential OpenDAL writes are not supported");
	}
	return Write(buffer_p, size_p);
}

void OpenDALFileHandle::Seek(idx_t offset_p) {
	EnsureOpen();
	position = offset_p;
}

idx_t OpenDALFileHandle::Tell() const {
	EnsureOpen();
	return position;
}

idx_t OpenDALFileHandle::Size() const {
	EnsureOpen();
	if (options.write) {
		return data.size();
	}
	return op->Stat(path).ContentLength();
}

void OpenDALFileHandle::Truncate(idx_t size_p) {
	EnsureWritable();
	if (size_p > NumericLimits<std::size_t>::Maximum()) {
		throw IOException("Truncate size exceeds addressable object size");
	}
	data.resize(static_cast<std::size_t>(size_p), '\0');
	if (position > size_p) {
		position = size_p;
	}
	dirty = true;
}

void OpenDALFileHandle::Flush() {
	EnsureOpen();
	if (options.write && dirty) {
		op->Write(path, data);
		dirty = false;
	}
}

void OpenDALFileHandle::Close() {
	if (closed) {
		return;
	}
	if (options.write && dirty) {
		op->Write(path, data);
		dirty = false;
	}
	reader.reset();
	closed = true;
}

} // namespace duckdb
