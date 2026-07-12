#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/types.hpp"
#include "duckdb/common/unordered_map.hpp"

namespace duckdb {

struct OpenDALUri {
	string authority;
	string root;
	bool trailing_slash = false;

	static OpenDALUri Parse(const string &uri_p, idx_t prefix_size_p);
	void Apply(const string &scheme_p, const string &uri_prefix_p, unordered_map<string, string> &config_p,
	           string &path_p) const;
};

} // namespace duckdb
