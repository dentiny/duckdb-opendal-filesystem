#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/types.hpp"
#include "duckdb/common/unordered_map.hpp"

namespace duckdb {

struct OpenDALUri {
	string authority;
	string root;
	string query;
	bool trailing_slash = false;

	static OpenDALUri Parse(const string &uri_p, idx_t prefix_size_p);
	//! Applies the backend-specific OpenDAL URI mapping. This adds values extracted from the URI
	//! (such as endpoint, bucket, or root) to config_p and replaces path_p with the object path
	//! relative to the configured OpenDAL operator root.
	void Apply(const string &scheme_p, const string &uri_prefix_p, unordered_map<string, string> &config_p,
	           string &path_p) const;
};

} // namespace duckdb
