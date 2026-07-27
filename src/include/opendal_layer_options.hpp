#pragma once

#include "duckdb/common/file_opener.hpp"
#include "duckdb/common/unique_ptr.hpp"
#include "duckdb/common/vector.hpp"

#include "opendal_config.hpp"

#include <opendal.hpp>

namespace duckdb {

struct OpenDALLayerOptions {
	static OpenDALLayerOptions FromSettings(optional_ptr<FileOpener> opener_p);
	vector<unique_ptr<opendal::OperatorOption>> ToOperatorOptions() const;

	bool timeout_layer_enabled = DEFAULT_OPENDAL_TIMEOUT_LAYER_ENABLED;
	uint64_t timeout_ms = DEFAULT_TIMEOUT_MS;
	uint64_t io_timeout_ms = DEFAULT_IO_TIMEOUT_MS;

	bool retry_layer_enabled = DEFAULT_OPENDAL_RETRY_LAYER_ENABLED;
	uint64_t retry_max_times = DEFAULT_RETRY_MAX_TIMES;
	double retry_factor = DEFAULT_RETRY_FACTOR;
	uint64_t retry_min_delay_ms = DEFAULT_RETRY_MIN_DELAY_MS;
	uint64_t retry_max_delay_ms = DEFAULT_RETRY_MAX_DELAY_MS;
	bool retry_jitter = DEFAULT_RETRY_JITTER;
};

} // namespace duckdb
