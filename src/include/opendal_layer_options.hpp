#pragma once

#include "duckdb/common/file_opener.hpp"

#include "opendal_config.hpp"

#include <opendal.hpp>

namespace duckdb {

struct OpenDALLayerOptions {
	static OpenDALLayerOptions FromSettings(optional_ptr<FileOpener> opener_p);
	void Validate() const;
	std::vector<std::unique_ptr<opendal::OperatorOption>> ToOperatorOptions() const;

	uint64_t timeout_ms = opendal_config::DEFAULT_TIMEOUT_MS;
	uint64_t io_timeout_ms = opendal_config::DEFAULT_IO_TIMEOUT_MS;
	uint64_t retry_max_times = opendal_config::DEFAULT_RETRY_MAX_TIMES;
	double retry_factor = opendal_config::DEFAULT_RETRY_FACTOR;
	uint64_t retry_min_delay_ms = opendal_config::DEFAULT_RETRY_MIN_DELAY_MS;
	uint64_t retry_max_delay_ms = opendal_config::DEFAULT_RETRY_MAX_DELAY_MS;
	bool retry_jitter = opendal_config::DEFAULT_RETRY_JITTER;
};

} // namespace duckdb
