#define DUCKDB_EXTENSION_MAIN

#include "duckdb_opendalfs_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/scalar_function.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

#include <opendal.hpp>

namespace duckdb {

inline void DuckDBOpenDALFSScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		return StringVector::AddString(result, "...........🦆 " + name.GetString());
	});
}

inline void DuckDBOpenDALFSOpenDALAvailableScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		opendal::Operator op("memory");
		return StringVector::AddString(result, "DuckDBOpenDALFS " + name.GetString() + ", OpenDAL memory service is " +
		                                           (op.Available() ? "available" : "unavailable"));
	});
}

static void LoadInternal(ExtensionLoader &loader) {
	// Register a scalar function
	auto duckdb_opendalfs_scalar_function =
	    ScalarFunction("duckdb_opendalfs", {LogicalType::VARCHAR}, LogicalType::VARCHAR, DuckDBOpenDALFSScalarFun);

	loader.RegisterFunction(duckdb_opendalfs_scalar_function);

	// This small probe also ensures that OpenDAL's C++ and Rust objects are
	// retained and linked into both extension variants.
	auto duckdb_opendalfs_opendal_available_scalar_function =
	    ScalarFunction("duckdb_opendalfs_opendal_available", {LogicalType::VARCHAR}, LogicalType::VARCHAR,
	                   DuckDBOpenDALFSOpenDALAvailableScalarFun);
	loader.RegisterFunction(duckdb_opendalfs_opendal_available_scalar_function);
}

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
