#pragma once

#include <cstdint>
#include <string_view>

namespace duckdb {

inline constexpr std::string_view OPENDALFS_EXTENSION_NAME = "duckdb_opendalfs";

inline constexpr std::string_view OPENDAL_TIMEOUT_LAYER_ENABLED = "opendal_timeout_layer_enabled";
inline constexpr std::string_view OPENDAL_TIMEOUT_LAYER_ENABLED_DESCRIPTION = "Enable the OpenDAL timeout layer";
inline constexpr bool DEFAULT_OPENDAL_TIMEOUT_LAYER_ENABLED = true;

inline constexpr std::string_view OPENDAL_TIMEOUT = "opendal_timeout_ms";
inline constexpr std::string_view OPENDAL_TIMEOUT_DESCRIPTION = "OpenDAL control-operation timeout in milliseconds";
inline constexpr uint64_t DEFAULT_TIMEOUT_MS = 60000;

inline constexpr std::string_view OPENDAL_IO_TIMEOUT = "opendal_io_timeout_ms";
inline constexpr std::string_view OPENDAL_IO_TIMEOUT_DESCRIPTION = "OpenDAL I/O-operation timeout in milliseconds";
inline constexpr uint64_t DEFAULT_IO_TIMEOUT_MS = 10000;

inline constexpr std::string_view OPENDAL_RETRY_LAYER_ENABLED = "opendal_retry_layer_enabled";
inline constexpr std::string_view OPENDAL_RETRY_LAYER_ENABLED_DESCRIPTION = "Enable the OpenDAL retry layer";
inline constexpr bool DEFAULT_OPENDAL_RETRY_LAYER_ENABLED = true;

inline constexpr std::string_view OPENDAL_RETRY_MAX_TIMES = "opendal_retry_max_times";
inline constexpr std::string_view OPENDAL_RETRY_MAX_TIMES_DESCRIPTION =
    "Maximum number of OpenDAL retry backoff delays";
inline constexpr uint64_t DEFAULT_RETRY_MAX_TIMES = 3;

inline constexpr std::string_view OPENDAL_RETRY_FACTOR = "opendal_retry_factor";
inline constexpr std::string_view OPENDAL_RETRY_FACTOR_DESCRIPTION = "OpenDAL exponential retry factor";
inline constexpr double DEFAULT_RETRY_FACTOR = 2.0;

inline constexpr std::string_view OPENDAL_RETRY_MIN_DELAY = "opendal_retry_min_delay_ms";
inline constexpr std::string_view OPENDAL_RETRY_MIN_DELAY_DESCRIPTION = "Minimum OpenDAL retry delay in milliseconds";
inline constexpr uint64_t DEFAULT_RETRY_MIN_DELAY_MS = 1000;

inline constexpr std::string_view OPENDAL_RETRY_MAX_DELAY = "opendal_retry_max_delay_ms";
inline constexpr std::string_view OPENDAL_RETRY_MAX_DELAY_DESCRIPTION = "Maximum OpenDAL retry delay in milliseconds";
inline constexpr uint64_t DEFAULT_RETRY_MAX_DELAY_MS = 60000;

inline constexpr std::string_view OPENDAL_RETRY_JITTER = "opendal_retry_jitter";
inline constexpr std::string_view OPENDAL_RETRY_JITTER_DESCRIPTION = "Add jitter to OpenDAL retry delays";
inline constexpr bool DEFAULT_RETRY_JITTER = false;

} // namespace duckdb
