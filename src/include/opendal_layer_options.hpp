#pragma once

#include "duckdb/common/file_opener.hpp"

#include "opendal_operator.hpp"

namespace duckdb {

struct OpenDALLayerOptions {
	static constexpr const char *TIMEOUT = "opendal_timeout_ms";
	static constexpr const char *IO_TIMEOUT = "opendal_io_timeout_ms";
	static constexpr const char *RETRY_MAX_TIMES = "opendal_retry_max_times";
	static constexpr const char *RETRY_FACTOR = "opendal_retry_factor";
	static constexpr const char *RETRY_MIN_DELAY = "opendal_retry_min_delay_ms";
	static constexpr const char *RETRY_MAX_DELAY = "opendal_retry_max_delay_ms";
	static constexpr const char *RETRY_JITTER = "opendal_retry_jitter";

	static OpenDALLayerOptions FromSettings(optional_ptr<FileOpener> opener_p);
	void Validate() const;
	opendal::LayerOptions ToOperatorOptions() const;

	uint64_t timeout_ms = 60000;
	uint64_t io_timeout_ms = 10000;
	uint64_t retry_max_times = 3;
	double retry_factor = 2.0;
	uint64_t retry_min_delay_ms = 1000;
	uint64_t retry_max_delay_ms = 60000;
	bool retry_jitter = false;
};

} // namespace duckdb
