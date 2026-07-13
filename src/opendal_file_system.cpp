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
#include <string_view>
#include <utility>

namespace duckdb {
namespace {

idx_t ReadFromOpenDAL(opendal::Reader &reader_p, void *buffer_p, idx_t size_p) {
	idx_t total_read = 0;
	while (total_read < size_p) {
		const auto read_size = reader_p.Read(static_cast<char *>(buffer_p) + total_read,
		                                     static_cast<std::streamsize>(size_p - total_read));
		if (read_size <= 0) {
			break;
		}
		total_read += static_cast<idx_t>(read_size);
	}
	return total_read;
}

} // namespace

OpenDALFileSystem::OpenDALFileSystem(const unordered_map<string, string> &config_p) : config(config_p) {
}

unique_ptr<opendal::Operator> OpenDALFileSystem::CreateOperator(const string &uri_p, OpenDALPath &parsed_path_p,
                                                                optional_ptr<FileOpener> opener_p) const {
	if (!OpenDALPath::TryParse(uri_p, parsed_path_p)) {
		throw InvalidInputException("Unsupported OpenDAL path prefix: %s", uri_p);
	}
	auto result = config;
	for (const auto &entry : parsed_path_p.uri_config) {
		result[entry.first] = entry.second;
	}
	auto secret_manager = FileOpener::TryGetSecretManager(opener_p);
	auto transaction = FileOpener::TryGetCatalogTransaction(opener_p);
	if (!secret_manager || !transaction) {
		return make_uniq<opendal::Operator>(parsed_path_p.scheme, result);
	}

	if (parsed_path_p.secret_type == OpenDALSecretType::NONE) {
		return make_uniq<opendal::Operator>(parsed_path_p.scheme, result);
	}
	auto secret_match =
	    secret_manager->LookupSecret(*transaction, uri_p, OpenDALSecretTypeName(parsed_path_p.secret_type));
	if (!secret_match.HasMatch()) {
		return make_uniq<opendal::Operator>(parsed_path_p.scheme, result);
	}
	const auto &secret = dynamic_cast<const KeyValueSecret &>(*secret_match.secret_entry->secret);
	for (const auto &entry : secret.secret_map) {
		auto key = entry.first;
		if (key == "bearer_token") {
			key = "token";
		}
		result[key] = entry.second.ToString();
	}
	return make_uniq<opendal::Operator>(parsed_path_p.scheme, result);
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

	OpenDALPath parsed_path;
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	if (parsed_path.path.empty()) {
		throw InvalidInputException("OpenDAL object path must not be empty");
	}

	const bool exists = op->Exists(parsed_path.path);
	if (!exists && !options.create) {
		if (flags_p.ReturnNullIfNotExists()) {
			return nullptr;
		}
		throw IOException("OpenDAL object does not exist: %s", path_p);
	}
	if (options.read) {
		return make_uniq<OpenDALReadHandle>(*this, std::move(op), path_p, std::move(parsed_path.path), flags_p);
	}
	return make_uniq<OpenDALWriteHandle>(*this, std::move(op), path_p, std::move(parsed_path.path), flags_p);
}

bool OpenDALFileSystem::FileExists(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath parsed_path;
	if (!OpenDALPath::TryParse(path_p, parsed_path) || parsed_path.path.empty()) {
		return false;
	}
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	return op->Exists(parsed_path.path);
}

int64_t OpenDALFileSystem::GetFileSize(FileHandle &handle_p) {
	return handle_p.Cast<OpenDALFileHandle>().Size();
}

void OpenDALFileSystem::Read(FileHandle &handle_p, void *buffer_p, int64_t size_p, idx_t offset_p) {
	if (size_p < 0 || handle_p.Cast<OpenDALReadHandle>().Read(buffer_p, static_cast<idx_t>(size_p), offset_p) !=
	                      static_cast<idx_t>(size_p)) {
		throw IOException("Could not read the requested number of bytes from OpenDAL");
	}
}

void OpenDALFileSystem::Write(FileHandle &handle_p, void *buffer_p, int64_t size_p, idx_t offset_p) {
	if (size_p < 0 || handle_p.Cast<OpenDALWriteHandle>().Write(buffer_p, static_cast<idx_t>(size_p), offset_p) !=
	                      static_cast<idx_t>(size_p)) {
		throw IOException("Could not write the requested number of bytes to OpenDAL");
	}
}

int64_t OpenDALFileSystem::Read(FileHandle &handle_p, void *buffer_p, int64_t size_p) {
	if (size_p < 0) {
		throw InvalidInputException("OpenDAL read size must not be negative");
	}
	return handle_p.Cast<OpenDALReadHandle>().Read(buffer_p, static_cast<idx_t>(size_p));
}

int64_t OpenDALFileSystem::Write(FileHandle &handle_p, void *buffer_p, int64_t size_p) {
	if (size_p < 0) {
		throw InvalidInputException("OpenDAL write size must not be negative");
	}
	return handle_p.Cast<OpenDALWriteHandle>().Write(buffer_p, static_cast<idx_t>(size_p));
}

bool OpenDALFileSystem::Trim(FileHandle &handle_p, idx_t offset_p, idx_t length_p) {
	return false;
}

FileMetadata OpenDALFileSystem::Stats(FileHandle &handle_p) {
	auto &handle = handle_p.Cast<OpenDALFileHandle>();
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
	handle_p.Cast<OpenDALReadHandle>().Seek(0);
}

void OpenDALFileSystem::Seek(FileHandle &handle_p, idx_t location_p) {
	handle_p.Cast<OpenDALReadHandle>().Seek(location_p);
}

idx_t OpenDALFileSystem::SeekPosition(FileHandle &handle_p) {
	if (handle_p.GetFlags().OpenForReading()) {
		return handle_p.Cast<OpenDALReadHandle>().Tell();
	}
	return handle_p.Cast<OpenDALWriteHandle>().Tell();
}

void OpenDALFileSystem::Truncate(FileHandle &handle_p, int64_t size_p) {
	throw NotImplementedException("OpenDAL truncate is not supported");
}

void OpenDALFileSystem::CreateDirectory(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath parsed_path;
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	if (parsed_path.path.empty()) {
		throw InvalidInputException("OpenDAL directory path must not be empty");
	}
	op->CreateDir(parsed_path.path);
}

void OpenDALFileSystem::CreateDirectoriesRecursive(const string &path_p, optional_ptr<FileOpener> opener_p) {
	CreateDirectory(path_p, opener_p);
}

bool OpenDALFileSystem::DirectoryExists(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath parsed_path;
	if (!OpenDALPath::TryParse(path_p, parsed_path) || parsed_path.path.empty()) {
		return false;
	}
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	return op->Stat(parsed_path.path).IsDir();
}

bool OpenDALFileSystem::ListFiles(const string &path_p, const std::function<void(const string &, bool)> &callback_p,
                                  FileOpener *opener_p) {
	OpenDALPath parsed_path;
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	if (parsed_path.path.empty()) {
		throw InvalidInputException("OpenDAL directory path must not be empty");
	}
	auto entries = op->List(parsed_path.path);
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
		auto op = CreateOperator(source_p, source, opener_p);
		op->Rename(source.path, target.path);
		return;
	}

	// Fallback to read and write.
	// TODO(hjiang): parallel read and write.
	auto source_op = CreateOperator(source_p, source, opener_p);
	auto target_op = CreateOperator(target_p, target, opener_p);
	auto data = source_op->Read(source.path);
	target_op->Write(target.path, data);
	source_op->Remove(source.path);
}

void OpenDALFileSystem::RemoveDirectory(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath parsed_path;
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	if (parsed_path.path.empty()) {
		throw InvalidInputException("OpenDAL directory path must not be empty");
	}
	op->Remove(parsed_path.path);
}

void OpenDALFileSystem::RemoveFile(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath parsed_path;
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	if (parsed_path.path.empty()) {
		throw InvalidInputException("OpenDAL object path must not be empty");
	}
	op->Remove(parsed_path.path);
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
		auto &parsed_path = paths[index];
		auto op = CreateOperator(paths_p[index], parsed_path, opener_p);
		op->Remove(parsed_path.path);
	}
}

bool OpenDALFileSystem::TryRemoveFile(const string &path_p, optional_ptr<FileOpener> opener_p) {
	OpenDALPath parsed_path;
	if (!OpenDALPath::TryParse(path_p, parsed_path) || parsed_path.path.empty()) {
		return false;
	}
	auto op = CreateOperator(path_p, parsed_path, opener_p);
	op->Remove(parsed_path.path);
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
                                     string path_p, FileOpenFlags flags_p)
    : FileHandle(fs_p, std::move(full_path_p), flags_p), op(std::move(op_p)), path(std::move(path_p)) {
}

OpenDALFileHandle::~OpenDALFileHandle() noexcept {
	Close();
}

void OpenDALFileHandle::EnsureOpen() const {
	if (closed) {
		throw IOException("OpenDAL file handle is closed");
	}
}

idx_t OpenDALFileHandle::Size() const {
	EnsureOpen();
	return op->Stat(path).ContentLength();
}

void OpenDALFileHandle::Flush() {
	EnsureOpen();
}

void OpenDALFileHandle::Close() {
	closed = true;
}

OpenDALReadHandle::OpenDALReadHandle(OpenDALFileSystem &fs_p, unique_ptr<opendal::Operator> op_p, string full_path_p,
                                     string path_p, FileOpenFlags flags_p)
    : OpenDALFileHandle(fs_p, std::move(op_p), std::move(full_path_p), std::move(path_p), flags_p),
      reader(make_uniq<opendal::Reader>(op->GetReader(path))) {
}

idx_t OpenDALReadHandle::ReadAt(void *buffer_p, idx_t size_p, idx_t offset_p) {
	if (size_p == 0) {
		return 0;
	}
	if (!buffer_p) {
		throw InvalidInputException("read buffer must not be null");
	}

	if (offset_p > static_cast<idx_t>(NumericLimits<std::streamoff>::Maximum())) {
		throw IOException("Read offset exceeds OpenDAL reader range");
	}
	if (size_p > static_cast<idx_t>(NumericLimits<std::streamsize>::Maximum())) {
		throw IOException("Read size exceeds OpenDAL reader range");
	}
	auto positional_reader = op->GetReader(path);
	if (offset_p != 0) {
		positional_reader.Seek(static_cast<std::streamoff>(offset_p), std::ios_base::beg);
	}
	return ReadFromOpenDAL(positional_reader, buffer_p, size_p);
}

idx_t OpenDALReadHandle::Read(void *buffer_p, idx_t size_p) {
	EnsureOpen();
	if (size_p == 0) {
		return 0;
	}
	if (!buffer_p) {
		throw InvalidInputException("read buffer must not be null");
	}
	if (position > static_cast<idx_t>(NumericLimits<std::streamoff>::Maximum())) {
		throw IOException("Read offset exceeds OpenDAL reader range");
	}
	if (size_p > static_cast<idx_t>(NumericLimits<std::streamsize>::Maximum())) {
		throw IOException("Read size exceeds OpenDAL reader range");
	}
	const auto read_size = ReadFromOpenDAL(*reader, buffer_p, size_p);
	position += read_size;
	return read_size;
}

idx_t OpenDALReadHandle::Read(void *buffer_p, idx_t size_p, idx_t offset_p) {
	EnsureOpen();
	return ReadAt(buffer_p, size_p, offset_p);
}

void OpenDALReadHandle::Seek(idx_t offset_p) {
	EnsureOpen();
	if (offset_p > static_cast<idx_t>(NumericLimits<std::streamoff>::Maximum())) {
		throw IOException("Seek offset exceeds OpenDAL reader range");
	}
	reader->Seek(static_cast<std::streamoff>(offset_p), std::ios_base::beg);
	position = offset_p;
}

idx_t OpenDALReadHandle::Tell() const {
	EnsureOpen();
	return position;
}

void OpenDALReadHandle::Close() {
	if (closed) {
		return;
	}
	reader.reset();
	OpenDALFileHandle::Close();
}

OpenDALWriteHandle::OpenDALWriteHandle(OpenDALFileSystem &fs_p, unique_ptr<opendal::Operator> op_p, string full_path_p,
                                       string path_p, FileOpenFlags flags_p)
    : OpenDALFileHandle(fs_p, std::move(op_p), std::move(full_path_p), std::move(path_p), flags_p) {
	op->Write(path, std::string_view());
}

idx_t OpenDALWriteHandle::WriteAt(const void *buffer_p, idx_t size_p, idx_t offset_p) {
	if (size_p == 0) {
		return 0;
	}
	if (!buffer_p) {
		throw InvalidInputException("write buffer must not be null");
	}
	if (offset_p != 0) {
		throw IOException("Multiple OpenDAL write calls require streaming writer support from the C++ binding");
	}
	op->Write(path, std::string_view(static_cast<const char *>(buffer_p), size_p));
	return size_p;
}

idx_t OpenDALWriteHandle::Write(const void *buffer_p, idx_t size_p) {
	return Write(buffer_p, size_p, position);
}

idx_t OpenDALWriteHandle::Write(const void *buffer_p, idx_t size_p, idx_t offset_p) {
	EnsureOpen();
	if (offset_p != position) {
		throw IOException("Non-sequential OpenDAL writes are not supported");
	}
	const auto write_size = WriteAt(buffer_p, size_p, offset_p);
	position += write_size;
	return write_size;
}

idx_t OpenDALWriteHandle::Tell() const {
	EnsureOpen();
	return position;
}

} // namespace duckdb
