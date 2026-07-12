#pragma once

#include "duckdb/common/string.hpp"

namespace duckdb {

struct OpenDALPath {
	string scheme;
	string path;
	string endpoint;

	static bool TryParse(const string &path_p, OpenDALPath &result_p);
};

} // namespace duckdb
