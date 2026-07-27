#include "opendal_layer_options.hpp"

#include "duckdb/common/exception.hpp"

namespace duckdb {
namespace {

template <class T>
void ReadSetting(optional_ptr<FileOpener> opener_p, const char *name_p, T &target_p) {
	FileOpener::TryGetCurrentSetting(opener_p, name_p, target_p, nullptr);
}

} // namespace

OpenDALLayerOptions OpenDALLayerOptions::FromSettings(optional_ptr<FileOpener> opener_p) {
	OpenDALLayerOptions result;
	ReadSetting(opener_p, OPENDAL_TIMEOUT_LAYER_ENABLED, result.timeout_layer_enabled);
	if (result.timeout_layer_enabled) {
		ReadSetting(opener_p, OPENDAL_TIMEOUT, result.timeout_ms);
		ReadSetting(opener_p, OPENDAL_IO_TIMEOUT, result.io_timeout_ms);
	}

	ReadSetting(opener_p, OPENDAL_RETRY_LAYER_ENABLED, result.retry_layer_enabled);
	if (result.retry_layer_enabled) {
		ReadSetting(opener_p, OPENDAL_RETRY_MAX_TIMES, result.retry_max_times);
		ReadSetting(opener_p, OPENDAL_RETRY_FACTOR, result.retry_factor);
		ReadSetting(opener_p, OPENDAL_RETRY_MIN_DELAY, result.retry_min_delay_ms);
		ReadSetting(opener_p, OPENDAL_RETRY_MAX_DELAY, result.retry_max_delay_ms);
		ReadSetting(opener_p, OPENDAL_RETRY_JITTER, result.retry_jitter);
		if (result.retry_min_delay_ms > result.retry_max_delay_ms) {
			throw InvalidInputException("opendal_retry_min_delay_ms must not exceed opendal_retry_max_delay_ms");
		}
	}
	return result;
}

vector<unique_ptr<opendal::OperatorOption>> OpenDALLayerOptions::ToOperatorOptions() const {
	vector<unique_ptr<opendal::OperatorOption>> result;
	if (timeout_layer_enabled) {
		auto timeout =
		    opendal::WithTimeout(std::chrono::milliseconds(timeout_ms), std::chrono::milliseconds(io_timeout_ms));
		result.push_back(unique_ptr<opendal::OperatorOption>(timeout.release()));
	}
	if (retry_layer_enabled) {
		opendal::RetryConfig retry;
		retry.max_times = retry_max_times;
		retry.factor = static_cast<float>(retry_factor);
		retry.min_delay = std::chrono::milliseconds(retry_min_delay_ms);
		retry.max_delay = std::chrono::milliseconds(retry_max_delay_ms);
		retry.jitter = retry_jitter;
		auto retry_option = opendal::WithRetry(retry);
		result.push_back(unique_ptr<opendal::OperatorOption>(retry_option.release()));
	}
	return result;
}

} // namespace duckdb
