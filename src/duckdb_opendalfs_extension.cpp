#define DUCKDB_EXTENSION_MAIN

#include "duckdb_opendalfs_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/opener_file_system.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "opendal_file_system.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

#include <opendal.hpp>

namespace duckdb {

namespace {
void LoadInternal(ExtensionLoader &loader) {
	auto &instance = loader.GetDatabaseInstance();
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
