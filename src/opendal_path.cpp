#include "opendal_path.hpp"

#include <string_view>

namespace duckdb {
namespace {

struct OpenDALPrefix {
	std::string_view prefix;
	std::string_view scheme;
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

} // namespace

bool OpenDALPath::TryParse(const string &path_p, OpenDALPath &result_p) {
	for (const auto &entry : OPENDAL_PREFIXES) {
		if (path_p.compare(0, entry.prefix.size(), entry.prefix.data(), entry.prefix.size()) == 0) {
			result_p.scheme.assign(entry.scheme.data(), entry.scheme.size());
			result_p.path = path_p.substr(entry.prefix.size());
			return true;
		}
	}
	return false;
}

} // namespace duckdb
