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

namespace duckdb {

namespace {
void LoadInternal(ExtensionLoader &loader) {
	RegisterOpenDALSecrets(loader);

	auto &instance = loader.GetDatabaseInstance();
	auto &config = DBConfig::GetConfig(instance);
	config.AddExtensionOption(opendal_config::TIMEOUT, opendal_config::TIMEOUT_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(opendal_config::DEFAULT_TIMEOUT_MS));
	config.AddExtensionOption(opendal_config::IO_TIMEOUT, opendal_config::IO_TIMEOUT_DESCRIPTION, LogicalType::UBIGINT,
	                          Value::UBIGINT(opendal_config::DEFAULT_IO_TIMEOUT_MS));
	config.AddExtensionOption(opendal_config::RETRY_MAX_TIMES, opendal_config::RETRY_MAX_TIMES_DESCRIPTION,
	                          LogicalType::UBIGINT, Value::UBIGINT(opendal_config::DEFAULT_RETRY_MAX_TIMES));
	config.AddExtensionOption(opendal_config::RETRY_FACTOR, opendal_config::RETRY_FACTOR_DESCRIPTION,
	                          LogicalType::DOUBLE, Value::DOUBLE(opendal_config::DEFAULT_RETRY_FACTOR));
	config.AddExtensionOption(opendal_config::RETRY_MIN_DELAY, opendal_config::RETRY_MIN_DELAY_DESCRIPTION,
	                          LogicalType::UBIGINT, Value::UBIGINT(opendal_config::DEFAULT_RETRY_MIN_DELAY_MS));
	config.AddExtensionOption(opendal_config::RETRY_MAX_DELAY, opendal_config::RETRY_MAX_DELAY_DESCRIPTION,
	                          LogicalType::UBIGINT, Value::UBIGINT(opendal_config::DEFAULT_RETRY_MAX_DELAY_MS));
	config.AddExtensionOption(opendal_config::RETRY_JITTER, opendal_config::RETRY_JITTER_DESCRIPTION,
	                          LogicalType::BOOLEAN, Value::BOOLEAN(opendal_config::DEFAULT_RETRY_JITTER));
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
