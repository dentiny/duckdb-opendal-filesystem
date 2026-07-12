#include "opendal_path.hpp"
#include "opendal_uri.hpp"

#include "duckdb/common/string_util.hpp"

#include <string_view>

namespace duckdb {
namespace {

struct OpenDALPrefix {
	std::string_view prefix;
	std::string_view scheme;
	std::string_view secret_type;
};

constexpr OpenDALPrefix OPENDAL_PREFIXES[] = {
    {"memory://", "memory", ""},
    {"s3://", "s3", "opendal_s3"},
    {"s3a://", "s3", "opendal_s3"},
    {"s3n://", "s3", "opendal_s3"},
    {"gs://", "gcs", "opendal_gcs"},
    {"gcs://", "gcs", "opendal_gcs"},
    {"az://", "azblob", "opendal_azblob"},
    {"azblob://", "azblob", "opendal_azblob"},
    {"abfs://", "azdls", "opendal_azdls"},
    {"abfss://", "azdls", "opendal_azdls"},
    {"azdls://", "azdls", "opendal_azdls"},
    {"azfile://", "azfile", "opendal_azfile"},
    {"http://", "http", "http"},
    {"https://", "http", "http"},
    {"aliyun-drive://", "aliyun-drive", "opendal_aliyun_drive"},
    {"b2://", "b2", "opendal_b2"},
    {"cos://", "cos", "opendal_cos"},
    {"dbfs://", "dbfs", "opendal_dbfs"},
    {"dropbox://", "dropbox", "opendal_dropbox"},
    {"ftp://", "ftp", "opendal_ftp"},
    {"gdrive://", "gdrive", "opendal_gdrive"},
    {"github://", "github", "opendal_github"},
    {"hf://", "hf", "opendal_hf"},
    {"huggingface://", "huggingface", "opendal_huggingface"},
    {"ipfs://", "ipfs", "opendal_ipfs"},
    {"koofr://", "koofr", "opendal_koofr"},
    {"lakefs://", "lakefs", "opendal_lakefs"},
    {"obs://", "obs", "opendal_obs"},
    {"onedrive://", "onedrive", "opendal_onedrive"},
    {"oss://", "oss", "opendal_oss"},
    {"pcloud://", "pcloud", "opendal_pcloud"},
    {"seafile://", "seafile", "opendal_seafile"},
    {"sftp://", "sftp", "opendal_sftp"},
    {"swift://", "swift", "opendal_swift"},
    {"tos://", "tos", "opendal_tos"},
    {"upyun://", "upyun", "opendal_upyun"},
    {"vercel-artifacts://", "vercel-artifacts", "opendal_vercel_artifacts"},
    {"vercel-blob://", "vercel-blob", "opendal_vercel_blob"},
    {"webdav://", "webdav", "opendal_webdav"},
    {"yandex-disk://", "yandex-disk", "opendal_yandex_disk"},
};

} // namespace

bool OpenDALPath::TryParse(const string &path_p, OpenDALPath &result_p) {
	for (const auto &entry : OPENDAL_PREFIXES) {
		if (!StringUtil::StartsWith(path_p, string(entry.prefix))) {
			continue;
		}
		result_p.scheme.assign(entry.scheme.data(), entry.scheme.size());
		result_p.secret_type.assign(entry.secret_type.data(), entry.secret_type.size());
		result_p.uri_config.clear();
		const auto uri = OpenDALUri::Parse(path_p, entry.prefix.size());
		uri.Apply(result_p.scheme, string(entry.prefix), result_p.uri_config, result_p.path);
		return true;
	}
	return false;
}

} // namespace duckdb
