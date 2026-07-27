#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"

#include <chrono>
#include <ios>
#include <optional>
#include <string_view>

struct opendal_operator;
struct opendal_reader;

namespace opendal {

struct LayerOptions {
	uint64_t timeout_ms = 60000;
	uint64_t io_timeout_ms = 10000;
	uint64_t retry_max_times = 3;
	float retry_factor = 2.0F;
	uint64_t retry_min_delay_ms = 1000;
	uint64_t retry_max_delay_ms = 60000;
	bool retry_jitter = false;
};

struct Entry {
	duckdb::string path;
};

class Metadata {
public:
	uint64_t ContentLength() const;
	bool IsFile() const;
	bool IsDir() const;
	std::optional<duckdb::string> Etag() const;
	std::optional<std::chrono::system_clock::time_point> LastModified() const;

private:
	friend class Operator;
	uint64_t content_length = 0;
	bool is_file = false;
	bool is_dir = false;
	std::optional<duckdb::string> etag;
	std::optional<std::chrono::system_clock::time_point> last_modified;
};

class Reader {
public:
	explicit Reader(opendal_reader *reader_p = nullptr);
	~Reader() noexcept;
	Reader(Reader &&other_p) noexcept;
	Reader &operator=(Reader &&other_p) noexcept;
	Reader(const Reader &) = delete;
	Reader &operator=(const Reader &) = delete;

	std::streamsize Read(void *buffer_p, std::streamsize size_p);
	std::streamoff Seek(std::streamoff offset_p, std::ios_base::seekdir direction_p);

private:
	opendal_reader *reader;
};

class Operator {
public:
	explicit Operator(const duckdb::string &scheme_p,
	                  const duckdb::unordered_map<duckdb::string, duckdb::string> &config_p = {},
	                  const LayerOptions &layers_p = {});
	~Operator() noexcept;
	Operator(const Operator &) = delete;
	Operator &operator=(const Operator &) = delete;

	bool Exists(const duckdb::string &path_p) const;
	Metadata Stat(const duckdb::string &path_p) const;
	Reader GetReader(const duckdb::string &path_p) const;
	duckdb::string Read(const duckdb::string &path_p) const;
	void Write(const duckdb::string &path_p, std::string_view data_p) const;
	void CreateDir(const duckdb::string &path_p) const;
	duckdb::vector<Entry> List(const duckdb::string &path_p) const;
	void Rename(const duckdb::string &source_p, const duckdb::string &target_p) const;
	void Remove(const duckdb::string &path_p) const;

private:
	opendal_operator *op;
};

} // namespace opendal
