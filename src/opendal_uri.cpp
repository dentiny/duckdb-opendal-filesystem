#include "opendal_uri.hpp"

#include "duckdb/common/string_util.hpp"

namespace duckdb {
namespace {

string TakeSegment(string &path_p) {
	const auto slash = path_p.find('/');
	auto result = path_p.substr(0, slash);
	path_p = slash == string::npos ? string() : path_p.substr(slash + 1);
	return result;
}

bool IsBucketService(const string &scheme_p) {
	return scheme_p == "s3" || scheme_p == "gcs" || scheme_p == "b2" || scheme_p == "cos" || scheme_p == "obs" ||
	       scheme_p == "oss" || scheme_p == "tos" || scheme_p == "upyun";
}

bool IsRootOnlyService(const string &scheme_p) {
	return scheme_p == "dropbox" || scheme_p == "gdrive" || scheme_p == "vercel-blob" || scheme_p == "yandex-disk";
}

} // namespace

OpenDALUri OpenDALUri::Parse(const string &uri_p, idx_t prefix_size_p) {
	OpenDALUri result;
	const auto slash = uri_p.find('/', prefix_size_p);
	const auto authority_size = slash == string::npos ? uri_p.size() - prefix_size_p : slash - prefix_size_p;
	result.authority = uri_p.substr(prefix_size_p, authority_size);
	result.root = slash == string::npos ? string() : uri_p.substr(slash + 1);
	const auto query = result.root.find('?');
	if (query != string::npos) {
		result.query = result.root.substr(query + 1);
		result.root.resize(query);
	}
	result.root = StringUtil::URLDecode(result.root);
	result.trailing_slash = !uri_p.empty() && uri_p.back() == '/';
	return result;
}

void OpenDALUri::Apply(const string &scheme_p, const string &uri_prefix_p, unordered_map<string, string> &config_p,
                       string &path_p) const {
	path_p = root;
	if (scheme_p == "http") {
		config_p["endpoint"] = uri_prefix_p + authority;
		if ((authority == "github.com" || StringUtil::StartsWith(authority, "github.com:")) && query == "raw=true") {
			const auto blob = path_p.find("/blob/");
			if (blob != string::npos) {
				path_p.replace(blob, 6, "/raw/");
			}
		}
		return;
	}
	if (IsBucketService(scheme_p)) {
		config_p["bucket"] = authority;
		return;
	}
	if (scheme_p == "azblob") {
		config_p["container"] = authority;
		return;
	}
	if (scheme_p == "aliyun-drive") {
		config_p["drive_type"] = authority;
		return;
	}
	if (scheme_p == "dbfs" || scheme_p == "pcloud" || scheme_p == "webdav") {
		config_p["endpoint"] = "https://" + authority;
		return;
	}
	if (scheme_p == "ftp") {
		config_p["endpoint"] = "ftp://" + authority;
		return;
	}
	if (scheme_p == "ipfs") {
		config_p["endpoint"] = "http://" + authority;
		return;
	}
	if (scheme_p == "sftp") {
		config_p["endpoint"] = authority;
		return;
	}
	if (scheme_p == "azdls" || scheme_p == "azfile") {
		config_p["endpoint"] = "https://" + authority;
		config_p["account_name"] = authority.substr(0, authority.find('.'));
		config_p[scheme_p == "azdls" ? "filesystem" : "share_name"] = TakeSegment(path_p);
		return;
	}
	if (scheme_p == "github") {
		config_p["owner"] = authority;
		config_p["repo"] = TakeSegment(path_p);
		return;
	}
	if (scheme_p == "koofr") {
		config_p["endpoint"] = "https://" + authority;
		config_p["email"] = TakeSegment(path_p);
		return;
	}
	if (scheme_p == "lakefs") {
		config_p["endpoint"] = "https://" + authority;
		config_p["repository"] = TakeSegment(path_p);
		config_p["branch"] = TakeSegment(path_p);
		return;
	}
	if (scheme_p == "seafile") {
		config_p["endpoint"] = "https://" + authority;
		config_p["repo_name"] = TakeSegment(path_p);
		return;
	}
	if (scheme_p == "swift") {
		config_p["endpoint"] = "https://" + authority;
		config_p["container"] = TakeSegment(path_p);
		return;
	}
	if (scheme_p == "onedrive") {
		TakeSegment(path_p);
		return;
	}
	if (scheme_p == "hf" || scheme_p == "huggingface") {
		config_p["repo_type"] = authority == "datasets" ? "dataset" : authority == "spaces" ? "space" : "model";
		auto owner = TakeSegment(path_p);
		auto repo = TakeSegment(path_p);
		const auto revision = repo.find('@');
		if (revision != string::npos) {
			config_p["revision"] = repo.substr(revision + 1);
			repo.resize(revision);
		}
		config_p["repo_id"] = owner + "/" + repo;
		return;
	}
	if (IsRootOnlyService(scheme_p)) {
		return;
	}
	path_p = authority;
	if (!root.empty()) {
		path_p += "/" + root;
	}
	if (trailing_slash && !path_p.empty() && path_p.back() != '/') {
		path_p += '/';
	}
}

} // namespace duckdb
