#define DUCKDB_EXTENSION_MAIN

#include "duckdb_opendalfs_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "opendal_file_system.hpp"
#include "opendal_layer_options.hpp"
#include "opendal_secret.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

namespace duckdb {

namespace {
void LoadInternal(ExtensionLoader &loader) {
	RegisterOpenDALSecrets(loader);

	auto &instance = loader.GetDatabaseInstance();
	auto &config = DBConfig::GetConfig(instance);
	config.AddExtensionOption(OpenDALLayerOptions::TIMEOUT, "OpenDAL control-operation timeout in milliseconds",
	                          LogicalType::UBIGINT, Value::UBIGINT(60000));
	config.AddExtensionOption(OpenDALLayerOptions::IO_TIMEOUT, "OpenDAL I/O-operation timeout in milliseconds",
	                          LogicalType::UBIGINT, Value::UBIGINT(10000));
	config.AddExtensionOption(OpenDALLayerOptions::RETRY_MAX_TIMES, "Maximum number of OpenDAL retry backoff delays",
	                          LogicalType::UBIGINT, Value::UBIGINT(3));
	config.AddExtensionOption(OpenDALLayerOptions::RETRY_FACTOR, "OpenDAL exponential retry factor",
	                          LogicalType::DOUBLE, Value::DOUBLE(2.0));
	config.AddExtensionOption(OpenDALLayerOptions::RETRY_MIN_DELAY, "Minimum OpenDAL retry delay in milliseconds",
	                          LogicalType::UBIGINT, Value::UBIGINT(1000));
	config.AddExtensionOption(OpenDALLayerOptions::RETRY_MAX_DELAY, "Maximum OpenDAL retry delay in milliseconds",
	                          LogicalType::UBIGINT, Value::UBIGINT(60000));
	config.AddExtensionOption(OpenDALLayerOptions::RETRY_JITTER, "Add jitter to OpenDAL retry delays",
	                          LogicalType::BOOLEAN, Value::BOOLEAN(false));
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
