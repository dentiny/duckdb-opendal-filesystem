#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"

namespace duckdb {

struct OpenDALPath {
	string scheme;
	string secret_type;
	string path;
	unordered_map<string, string> uri_config;

	static bool TryParse(const string &path_p, OpenDALPath &result_p);
};

} // namespace duckdb
