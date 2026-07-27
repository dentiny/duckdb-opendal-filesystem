#include "opendal_operator.hpp"

#include "duckdb/common/exception.hpp"

#include <opendal.h>

#include <limits>

namespace opendal {
namespace {

duckdb::string ErrorMessage(opendal_error *error_p) {
	duckdb::string result(reinterpret_cast<const char *>(error_p->message.data), error_p->message.len);
	opendal_error_free(error_p);
	return result;
}

void ThrowIfError(opendal_error *error_p, const char *operation_p) {
	if (error_p) {
		throw duckdb::IOException("OpenDAL %s failed: %s", operation_p, ErrorMessage(error_p));
	}
}

uint64_t MillisecondsToNanoseconds(uint64_t value_p) {
	if (value_p > std::numeric_limits<uint64_t>::max() / 1000000) {
		throw duckdb::InvalidInputException("OpenDAL layer duration is too large");
	}
	return value_p * 1000000;
}

} // namespace

uint64_t Metadata::ContentLength() const {
	return content_length;
}
bool Metadata::IsFile() const {
	return is_file;
}
bool Metadata::IsDir() const {
	return is_dir;
}
std::optional<duckdb::string> Metadata::Etag() const {
	return etag;
}
std::optional<std::chrono::system_clock::time_point> Metadata::LastModified() const {
	return last_modified;
}

Reader::Reader(opendal_reader *reader_p) : reader(reader_p) {
}
Reader::~Reader() noexcept {
	if (reader)
		opendal_reader_free(reader);
}
Reader::Reader(Reader &&other_p) noexcept : reader(other_p.reader) {
	other_p.reader = nullptr;
}
Reader &Reader::operator=(Reader &&other_p) noexcept {
	if (this != &other_p) {
		if (reader)
			opendal_reader_free(reader);
		reader = other_p.reader;
		other_p.reader = nullptr;
	}
	return *this;
}
std::streamsize Reader::Read(void *buffer_p, std::streamsize size_p) {
	auto result = opendal_reader_read(reader, reinterpret_cast<uint8_t *>(buffer_p), static_cast<uintptr_t>(size_p));
	ThrowIfError(result.error, "read");
	return static_cast<std::streamsize>(result.size);
}
std::streamoff Reader::Seek(std::streamoff offset_p, std::ios_base::seekdir direction_p) {
	const int32_t whence = direction_p == std::ios_base::beg ? 0 : direction_p == std::ios_base::cur ? 1 : 2;
	auto result = opendal_reader_seek(reader, offset_p, whence);
	ThrowIfError(result.error, "seek");
	return static_cast<std::streamoff>(result.pos);
}

Operator::Operator(const duckdb::string &scheme_p,
                   const duckdb::unordered_map<duckdb::string, duckdb::string> &config_p, const LayerOptions &layers_p)
    : op(nullptr) {
	auto options = opendal_operator_options_new();
	auto layers = opendal_operator_layers_new();
	for (const auto &entry : config_p) {
		opendal_operator_options_set(options, entry.first.c_str(), entry.second.c_str());
	}
	opendal_operator_layers_add_timeout(layers, MillisecondsToNanoseconds(layers_p.timeout_ms),
	                                    MillisecondsToNanoseconds(layers_p.io_timeout_ms));
	opendal_operator_layers_add_retry(layers, layers_p.retry_jitter, layers_p.retry_factor,
	                                  MillisecondsToNanoseconds(layers_p.retry_min_delay_ms),
	                                  MillisecondsToNanoseconds(layers_p.retry_max_delay_ms), layers_p.retry_max_times);
	auto result = opendal_operator_new_with_layers(scheme_p.c_str(), options, layers);
	opendal_operator_layers_free(layers);
	opendal_operator_options_free(options);
	ThrowIfError(result.error, "operator creation");
	op = result.op;
}
Operator::~Operator() noexcept {
	if (op)
		opendal_operator_free(op);
}
bool Operator::Exists(const duckdb::string &path_p) const {
	auto result = opendal_operator_exists(op, path_p.c_str());
	ThrowIfError(result.error, "exists");
	return result.exists;
}
Metadata Operator::Stat(const duckdb::string &path_p) const {
	auto result = opendal_operator_stat(op, path_p.c_str());
	ThrowIfError(result.error, "stat");
	Metadata metadata;
	metadata.content_length = opendal_metadata_content_length(result.meta);
	metadata.is_file = opendal_metadata_is_file(result.meta);
	metadata.is_dir = opendal_metadata_is_dir(result.meta);
	auto etag = opendal_metadata_etag(result.meta);
	if (etag) {
		metadata.etag = etag;
		opendal_string_free(etag);
	}
	const auto modified_ms = opendal_metadata_last_modified_ms(result.meta);
	if (modified_ms >= 0)
		metadata.last_modified = std::chrono::system_clock::time_point(std::chrono::milliseconds(modified_ms));
	opendal_metadata_free(result.meta);
	return metadata;
}
Reader Operator::GetReader(const duckdb::string &path_p) const {
	auto result = opendal_operator_reader(op, path_p.c_str());
	ThrowIfError(result.error, "reader creation");
	return Reader(result.reader);
}
duckdb::string Operator::Read(const duckdb::string &path_p) const {
	auto result = opendal_operator_read(op, path_p.c_str());
	ThrowIfError(result.error, "read");
	duckdb::string data(reinterpret_cast<const char *>(result.data.data), result.data.len);
	opendal_bytes_free(&result.data);
	return data;
}
void Operator::Write(const duckdb::string &path_p, std::string_view data_p) const {
	opendal_bytes bytes {reinterpret_cast<uint8_t *>(const_cast<char *>(data_p.data())), data_p.size(), data_p.size()};
	ThrowIfError(opendal_operator_write(op, path_p.c_str(), &bytes), "write");
}
void Operator::CreateDir(const duckdb::string &path_p) const {
	ThrowIfError(opendal_operator_create_dir(op, path_p.c_str()), "create directory");
}
duckdb::vector<Entry> Operator::List(const duckdb::string &path_p) const {
	auto result = opendal_operator_list(op, path_p.c_str());
	ThrowIfError(result.error, "list");
	duckdb::vector<Entry> entries;
	while (true) {
		auto next = opendal_lister_next(result.lister);
		ThrowIfError(next.error, "list next");
		if (!next.entry)
			break;
		auto path = opendal_entry_path(next.entry);
		entries.push_back({path});
		opendal_string_free(path);
		opendal_entry_free(next.entry);
	}
	opendal_lister_free(result.lister);
	return entries;
}
void Operator::Rename(const duckdb::string &source_p, const duckdb::string &target_p) const {
	ThrowIfError(opendal_operator_rename(op, source_p.c_str(), target_p.c_str()), "rename");
}
void Operator::Remove(const duckdb::string &path_p) const {
	ThrowIfError(opendal_operator_delete(op, path_p.c_str()), "delete");
}

} // namespace opendal
