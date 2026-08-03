#include "opendal_remove_utils.hpp"

#include "duckdb/common/exception.hpp"
#include "opendal_path.hpp"

#include <algorithm>
#include <utility>

namespace duckdb {
namespace {

void AppendKeyComponent(string &key_p, const string &component_p) {
	key_p += std::to_string(component_p.size());
	key_p += ':';
	key_p += component_p;
}

string CreateRemoveGroupKey(const string &scheme_p, const unordered_map<string, string> &config_p) {
	string result;
	AppendKeyComponent(result, scheme_p);

	vector<std::pair<string, string>> config_entries;
	config_entries.reserve(config_p.size());
	for (const auto &entry : config_p) {
		config_entries.emplace_back(entry.first, entry.second);
	}
	std::sort(config_entries.begin(), config_entries.end());
	for (const auto &entry : config_entries) {
		AppendKeyComponent(result, entry.first);
		AppendKeyComponent(result, entry.second);
	}
	return result;
}

} // namespace

vector<OpenDALRemoveGroup> GroupOpenDALRemovePaths(const vector<string> &paths_p,
                                                   const OpenDALOperatorConfigProvider &config_provider_p) {
	vector<OpenDALRemoveGroup> groups;
	// Maps from operator identity to the group index while `groups` preserves first-seen order.
	unordered_map<string, size_t> group_index;
	for (const auto &path_string : paths_p) {
		OpenDALPath path;
		if (!OpenDALPath::TryParse(path_string, path)) {
			throw InvalidInputException("Unsupported OpenDAL path prefix: %s", path_string);
		}
		if (path.path.empty()) {
			throw InvalidInputException("OpenDAL object path must not be empty");
		}
		auto operator_config = config_provider_p(path_string, path);
		auto group_key = CreateRemoveGroupKey(path.scheme, operator_config);
		auto group = group_index.find(group_key);
		if (group == group_index.end()) {
			const auto index = groups.size();
			group_index.emplace(std::move(group_key), index);
			OpenDALRemoveGroup remove_group;
			remove_group.scheme = std::move(path.scheme);
			remove_group.config = std::move(operator_config);
			remove_group.paths.push_back(std::move(path.path));
			groups.emplace_back(std::move(remove_group));
			continue;
		}
		groups[group->second].paths.emplace_back(std::move(path.path));
	}
	return groups;
}

} // namespace duckdb
