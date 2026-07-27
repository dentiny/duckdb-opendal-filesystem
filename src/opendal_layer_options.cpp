#include "opendal_layer_options.hpp"

#include "duckdb/common/exception.hpp"

#include <cmath>
#include <limits>

namespace duckdb {
namespace {

template <class T>
void ReadSetting(optional_ptr<FileOpener> opener_p, const char *name_p, T &target_p) {
	FileOpener::TryGetCurrentSetting(opener_p, name_p, target_p, nullptr);
}

} // namespace

OpenDALLayerOptions OpenDALLayerOptions::FromSettings(optional_ptr<FileOpener> opener_p) {
	OpenDALLayerOptions result;
	ReadSetting(opener_p, TIMEOUT, result.timeout_ms);
	ReadSetting(opener_p, IO_TIMEOUT, result.io_timeout_ms);
	ReadSetting(opener_p, RETRY_MAX_TIMES, result.retry_max_times);
	ReadSetting(opener_p, RETRY_FACTOR, result.retry_factor);
	ReadSetting(opener_p, RETRY_MIN_DELAY, result.retry_min_delay_ms);
	ReadSetting(opener_p, RETRY_MAX_DELAY, result.retry_max_delay_ms);
	ReadSetting(opener_p, RETRY_JITTER, result.retry_jitter);
	result.Validate();
	return result;
}

void OpenDALLayerOptions::Validate() const {
	constexpr uint64_t MAX_MILLISECONDS = std::numeric_limits<uint64_t>::max() / 1000000;
	if (timeout_ms == 0) {
		throw InvalidInputException("opendal_timeout_ms must be greater than zero");
	}
	if (io_timeout_ms == 0) {
		throw InvalidInputException("opendal_io_timeout_ms must be greater than zero");
	}
	if (!std::isfinite(retry_factor) || retry_factor < 1.0 ||
	    retry_factor > static_cast<double>(std::numeric_limits<float>::max())) {
		throw InvalidInputException("opendal_retry_factor must be finite and at least 1.0");
	}
	if (retry_min_delay_ms > retry_max_delay_ms) {
		throw InvalidInputException("opendal_retry_min_delay_ms must not exceed opendal_retry_max_delay_ms");
	}
	if (timeout_ms > MAX_MILLISECONDS || io_timeout_ms > MAX_MILLISECONDS || retry_min_delay_ms > MAX_MILLISECONDS ||
	    retry_max_delay_ms > MAX_MILLISECONDS) {
		throw InvalidInputException("OpenDAL layer duration exceeds the supported range");
	}
	if (retry_max_times > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
		throw InvalidInputException("opendal_retry_max_times exceeds the supported range");
	}
}

opendal::LayerOptions OpenDALLayerOptions::ToOperatorOptions() const {
	Validate();
	opendal::LayerOptions result;
	result.timeout_ms = timeout_ms;
	result.io_timeout_ms = io_timeout_ms;
	result.retry_max_times = retry_max_times;
	result.retry_factor = static_cast<float>(retry_factor);
	result.retry_min_delay_ms = retry_min_delay_ms;
	result.retry_max_delay_ms = retry_max_delay_ms;
	result.retry_jitter = retry_jitter;
	return result;
}

} // namespace duckdb
