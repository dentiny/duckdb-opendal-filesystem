#include "opendal_path.hpp"

#include "duckdb/common/string_util.hpp"

namespace duckdb {

struct OpenDALPrefix {
	const char *prefix;
	const char *scheme;
};

static constexpr OpenDALPrefix OPENDAL_PREFIXES[] = {
    {"memory://", "memory"},
    {"s3://", "s3"},
    {"s3a://", "s3"},
    {"s3n://", "s3"},
    {"gs://", "gcs"},
    {"gcs://", "gcs"},
    {"az://", "azblob"},
    {"azblob://", "azblob"},
    {"abfs://", "azdls"},
    {"abfss://", "azdls"},
    {"azdls://", "azdls"},
    {"azfile://", "azfile"},
    {"http://", "http"},
    {"https://", "http"},
    {"aliyun-drive://", "aliyun-drive"},
    {"b2://", "b2"},
    {"cos://", "cos"},
    {"dbfs://", "dbfs"},
    {"dropbox://", "dropbox"},
    {"ftp://", "ftp"},
    {"gdrive://", "gdrive"},
    {"github://", "github"},
    {"hf://", "hf"},
    {"huggingface://", "huggingface"},
    {"ipfs://", "ipfs"},
    {"koofr://", "koofr"},
    {"lakefs://", "lakefs"},
    {"obs://", "obs"},
    {"onedrive://", "onedrive"},
    {"oss://", "oss"},
    {"pcloud://", "pcloud"},
    {"seafile://", "seafile"},
    {"sftp://", "sftp"},
    {"swift://", "swift"},
    {"tos://", "tos"},
    {"upyun://", "upyun"},
    {"vercel-artifacts://", "vercel-artifacts"},
    {"vercel-blob://", "vercel-blob"},
    {"webdav://", "webdav"},
    {"yandex-disk://", "yandex-disk"},
};

bool OpenDALPath::TryParse(const string &path_p, OpenDALPath &result_p) {
	for (const auto &entry : OPENDAL_PREFIXES) {
		const string prefix(entry.prefix);
		if (StringUtil::StartsWith(path_p, prefix)) {
			result_p.scheme = entry.scheme;
			result_p.path = path_p.substr(prefix.size());
			return true;
		}
	}
	return false;
}

} // namespace duckdb
