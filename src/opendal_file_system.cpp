#include "opendal_file_system.hpp"

#include "duckdb/common/exception.hpp"
#include "duckdb/common/limits.hpp"

#include <opendal.hpp>

#include <algorithm>
#include <cstring>
#include <utility>

namespace duckdb {

OpenDALFileSystem::OpenDALFileSystem(string scheme_p, const unordered_map<string, string> &config_p)
    : scheme(std::move(scheme_p)), config(config_p) {
	auto op = make_uniq<opendal::Operator>(scheme, config);
	if (!op->Available()) {
		throw IOException("OpenDAL operator is unavailable");
	}
}

unique_ptr<OpenDALFileHandle> OpenDALFileSystem::Open(const string &path_p, OpenDALOpenOptions options_p) {
	if (path_p.empty()) {
		throw InvalidInputException("OpenDAL path must not be empty");
	}
	if (!options_p.IsValid()) {
		throw InvalidInputException("Invalid OpenDAL file open options");
	}

	auto op = make_uniq<opendal::Operator>(scheme, config);
	const bool exists = op->Exists(path_p);
	if (!exists && !options_p.create) {
		throw IOException("OpenDAL object does not exist: %s", path_p);
	}
	return make_uniq<OpenDALFileHandle>(std::move(op), path_p, options_p, exists);
}

bool OpenDALFileSystem::Exists(const string &path_p) const {
	auto op = make_uniq<opendal::Operator>(scheme, config);
	return op->Exists(path_p);
}

OpenDALFileHandle::OpenDALFileHandle(unique_ptr<opendal::Operator> op_p, string path_p, OpenDALOpenOptions options_p,
                                     bool exists_p)
    : op(std::move(op_p)), path(std::move(path_p)), options(options_p) {
	if (options.write) {
		if (exists_p && !options.truncate) {
			data = op->Read(path);
		}
		dirty = !exists_p || options.truncate;
		position = options.append ? data.size() : 0;
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
	const auto offset = options.append ? data.size() : position;
	const auto write_size = WriteAt(buffer_p, size_p, offset);
	position = offset + write_size;
	return write_size;
}

idx_t OpenDALFileHandle::Write(const void *buffer_p, idx_t size_p, idx_t offset_p) {
	EnsureWritable();
	return WriteAt(buffer_p, size_p, offset_p);
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
	reader.reset();
	closed = true;
}

} // namespace duckdb
