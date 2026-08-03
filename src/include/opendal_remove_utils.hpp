#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/vector.hpp"

#include <functional>

namespace duckdb {

struct OpenDALPath;

struct OpenDALRemoveGroup {
	string scheme;
	unordered_map<string, string> config;
	vector<string> paths;
};

using OpenDALOperatorConfigProvider = std::function<unordered_map<string, string>(const string &, const OpenDALPath &)>;

vector<OpenDALRemoveGroup> GroupOpenDALRemovePaths(const vector<string> &paths_p,
                                                   const OpenDALOperatorConfigProvider &config_provider_p);

} // namespace duckdb
