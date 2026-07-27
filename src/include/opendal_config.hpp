#pragma once

#include <cstdint>

namespace duckdb {
namespace opendal_config {

inline constexpr const char *TIMEOUT = "opendal_timeout_ms";
inline constexpr const char *TIMEOUT_DESCRIPTION = "OpenDAL control-operation timeout in milliseconds";
inline constexpr uint64_t DEFAULT_TIMEOUT_MS = 60000;

inline constexpr const char *IO_TIMEOUT = "opendal_io_timeout_ms";
inline constexpr const char *IO_TIMEOUT_DESCRIPTION = "OpenDAL I/O-operation timeout in milliseconds";
inline constexpr uint64_t DEFAULT_IO_TIMEOUT_MS = 10000;

inline constexpr const char *RETRY_MAX_TIMES = "opendal_retry_max_times";
inline constexpr const char *RETRY_MAX_TIMES_DESCRIPTION = "Maximum number of OpenDAL retry backoff delays";
inline constexpr uint64_t DEFAULT_RETRY_MAX_TIMES = 3;

inline constexpr const char *RETRY_FACTOR = "opendal_retry_factor";
inline constexpr const char *RETRY_FACTOR_DESCRIPTION = "OpenDAL exponential retry factor";
inline constexpr double DEFAULT_RETRY_FACTOR = 2.0;

inline constexpr const char *RETRY_MIN_DELAY = "opendal_retry_min_delay_ms";
inline constexpr const char *RETRY_MIN_DELAY_DESCRIPTION = "Minimum OpenDAL retry delay in milliseconds";
inline constexpr uint64_t DEFAULT_RETRY_MIN_DELAY_MS = 1000;

inline constexpr const char *RETRY_MAX_DELAY = "opendal_retry_max_delay_ms";
inline constexpr const char *RETRY_MAX_DELAY_DESCRIPTION = "Maximum OpenDAL retry delay in milliseconds";
inline constexpr uint64_t DEFAULT_RETRY_MAX_DELAY_MS = 60000;

inline constexpr const char *RETRY_JITTER = "opendal_retry_jitter";
inline constexpr const char *RETRY_JITTER_DESCRIPTION = "Add jitter to OpenDAL retry delays";
inline constexpr bool DEFAULT_RETRY_JITTER = false;

} // namespace opendal_config
} // namespace duckdb
