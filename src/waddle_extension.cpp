#define DUCKDB_EXTENSION_MAIN

#include "waddle_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/function/scalar_function.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

#include <opendal.hpp>

namespace duckdb {

inline void WaddleScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		return StringVector::AddString(result, "...........🦆 " + name.GetString());
	});
}

inline void WaddleOpenDALAvailableScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		opendal::Operator op("memory");
		return StringVector::AddString(result, "Waddle " + name.GetString() + ", OpenDAL memory service is " +
		                                           (op.Available() ? "available" : "unavailable"));
	});
}

static void LoadInternal(ExtensionLoader &loader) {
	// Register a scalar function
	auto waddle_scalar_function =
	    ScalarFunction("waddle", {LogicalType::VARCHAR}, LogicalType::VARCHAR, WaddleScalarFun);

	loader.RegisterFunction(waddle_scalar_function);

	// This small probe also ensures that OpenDAL's C++ and Rust objects are
	// retained and linked into both extension variants.
	auto waddle_opendal_available_scalar_function =
	    ScalarFunction("waddle_opendal_available", {LogicalType::VARCHAR}, LogicalType::VARCHAR,
	                   WaddleOpenDALAvailableScalarFun);
	loader.RegisterFunction(waddle_opendal_available_scalar_function);
}

void WaddleExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}
std::string WaddleExtension::Name() {
	return "waddle";
}

std::string WaddleExtension::Version() const {
#ifdef EXT_VERSION_WADDLE
	return EXT_VERSION_WADDLE;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(waddle, loader) {
	duckdb::LoadInternal(loader);
}
}
