#define DUCKDB_EXTENSION_MAIN

#include "duckdb_opendalfs_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "opendal_file_system.hpp"
#include "opendal_config.hpp"
#include "opendal_layer_options.hpp"
#include "opendal_secret.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

#include <cmath>
#include <limits>

namespace duckdb {

namespace {

constexpr uint64_t MAX_OPENDAL_LAYER_MILLISECONDS = std::numeric_limits<uint64_t>::max() / 1000000;

void ValidatePositiveMilliseconds(const char *name_p, const Value &parameter_p) {
	const auto value = parameter_p.GetValue<uint64_t>();
	if (value == 0) {
		throw InvalidInputException("%s must be greater than zero", name_p);
	}
	if (value > MAX_OPENDAL_LAYER_MILLISECONDS) {
		throw InvalidInputException("%s exceeds the supported range", name_p);
	}
}

void ValidateTimeout(ClientContext &, SetScope, Value &parameter_p) {
	ValidatePositiveMilliseconds(OPENDAL_TIMEOUT, parameter_p);
}

void ValidateIOTimeout(ClientContext &, SetScope, Value &parameter_p) {
	ValidatePositiveMilliseconds(OPENDAL_IO_TIMEOUT, parameter_p);
}

void ValidateRetryMaxTimes(ClientContext &, SetScope, Value &parameter_p) {
	const auto value = parameter_p.GetValue<uint64_t>();
	if (value == 0) {
		throw InvalidInputException("opendal_retry_max_times must be greater than zero");
	}
	if (value > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
		throw InvalidInputException("opendal_retry_max_times exceeds the supported range");
	}
}

void ValidateRetryFactor(ClientContext &, SetScope, Value &parameter_p) {
	const auto value = parameter_p.GetValue<double>();
	if (!std::isfinite(value) || value < 1.0 || value > static_cast<double>(std::numeric_limits<float>::max())) {
		throw InvalidInputException("opendal_retry_factor must be finite and at least 1.0");
	}
}

template <class T>
T GetCurrentSetting(ClientContext &context_p, const char *name_p, T default_p) {
	Value current_value;
	if (context_p.TryGetCurrentSetting(name_p, current_value)) {
		return current_value.GetValue<T>();
	}
	return default_p;
}

void ValidateRetryMinDelay(ClientContext &context_p, SetScope, Value &parameter_p) {
	ValidatePositiveMilliseconds(OPENDAL_RETRY_MIN_DELAY, parameter_p);
}

void ValidateRetryMaxDelay(ClientContext &context_p, SetScope, Value &parameter_p) {
	ValidatePositiveMilliseconds(OPENDAL_RETRY_MAX_DELAY, parameter_p);
}

void LoadInternal(ExtensionLoader &loader) {
	RegisterOpenDALSecrets(loader);

	auto &instance = loader.GetDatabaseInstance();
	auto &config = DBConfig::GetConfig(instance);

	// Timeout layer settings.
	config.AddExtensionOption(OPENDAL_TIMEOUT_LAYER_ENABLED, OPENDAL_TIMEOUT_LAYER_ENABLED_DESCRIPTION,
	                          LogicalType::BOOLEAN, Value::BOOLEAN(DEFAULT_OPENDAL_TIMEOUT_LAYER_ENABLED));
	config.AddExtensionOption(OPENDAL_TIMEOUT, OPENDAL_TIMEOUT_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(DEFAULT_TIMEOUT_MS), ValidateTimeout);
	config.AddExtensionOption(OPENDAL_IO_TIMEOUT, OPENDAL_IO_TIMEOUT_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(DEFAULT_IO_TIMEOUT_MS), ValidateIOTimeout);

	// Retry layer settings.
	config.AddExtensionOption(OPENDAL_RETRY_LAYER_ENABLED, OPENDAL_RETRY_LAYER_ENABLED_DESCRIPTION,
	                          LogicalType::BOOLEAN, Value::BOOLEAN(DEFAULT_OPENDAL_RETRY_LAYER_ENABLED));
	config.AddExtensionOption(OPENDAL_RETRY_MAX_TIMES, OPENDAL_RETRY_MAX_TIMES_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(DEFAULT_RETRY_MAX_TIMES), ValidateRetryMaxTimes);
	config.AddExtensionOption(OPENDAL_RETRY_FACTOR, OPENDAL_RETRY_FACTOR_DESCRIPTION, LogicalType::DOUBLE,
	                          Value::DOUBLE(DEFAULT_RETRY_FACTOR), ValidateRetryFactor);
	config.AddExtensionOption(OPENDAL_RETRY_MIN_DELAY, OPENDAL_RETRY_MIN_DELAY_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(DEFAULT_RETRY_MIN_DELAY_MS), ValidateRetryMinDelay);
	config.AddExtensionOption(OPENDAL_RETRY_MAX_DELAY, OPENDAL_RETRY_MAX_DELAY_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(DEFAULT_RETRY_MAX_DELAY_MS), ValidateRetryMaxDelay);
	config.AddExtensionOption(OPENDAL_RETRY_JITTER, OPENDAL_RETRY_JITTER_DESCRIPTION, LogicalType::BOOLEAN,
	                          Value::BOOLEAN(DEFAULT_RETRY_JITTER));
	auto &opener_filesystem = instance.GetFileSystem().Cast<OpenerFileSystem>();
	auto &vfs = opener_filesystem.GetFileSystem();
	vfs.RegisterSubSystem(make_uniq<OpenDALFileSystem>());
}
} // namespace

void DuckdbOpendalfsExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}
std::string DuckdbOpendalfsExtension::Name() {
	return "duckdb_opendalfs";
}

std::string DuckdbOpendalfsExtension::Version() const {
#ifdef EXT_VERSION_DUCKDB_OPENDALFS
	return EXT_VERSION_DUCKDB_OPENDALFS;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(duckdb_opendalfs, loader) {
	duckdb::LoadInternal(loader);
}
}
