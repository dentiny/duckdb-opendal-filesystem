#include "opendal_path.hpp"
#include "opendal_uri.hpp"

#include "duckdb/common/string_util.hpp"

#include <string_view>

namespace duckdb {
namespace {

struct OpenDALPrefix {
	std::string_view prefix;
	std::string_view scheme;
	OpenDALSecretType secret_type;
};

constexpr OpenDALPrefix OPENDAL_PREFIXES[] = {
    {"memory://", "memory", OpenDALSecretType::NONE},
    {"s3://", "s3", OpenDALSecretType::S3},
    {"s3a://", "s3", OpenDALSecretType::S3},
    {"s3n://", "s3", OpenDALSecretType::S3},
    {"gs://", "gcs", OpenDALSecretType::GCS},
    {"gcs://", "gcs", OpenDALSecretType::GCS},
    {"az://", "azblob", OpenDALSecretType::AZBLOB},
    {"azblob://", "azblob", OpenDALSecretType::AZBLOB},
    {"abfs://", "azdls", OpenDALSecretType::AZDLS},
    {"abfss://", "azdls", OpenDALSecretType::AZDLS},
    {"azdls://", "azdls", OpenDALSecretType::AZDLS},
    {"azfile://", "azfile", OpenDALSecretType::AZFILE},
    {"http://", "http", OpenDALSecretType::HTTP},
    {"https://", "http", OpenDALSecretType::HTTP},
    {"aliyun-drive://", "aliyun-drive", OpenDALSecretType::ALIYUN_DRIVE},
    {"b2://", "b2", OpenDALSecretType::B2},
    {"cos://", "cos", OpenDALSecretType::COS},
    {"dbfs://", "dbfs", OpenDALSecretType::DBFS},
    {"dropbox://", "dropbox", OpenDALSecretType::DROPBOX},
    {"ftp://", "ftp", OpenDALSecretType::FTP},
    {"gdrive://", "gdrive", OpenDALSecretType::GDRIVE},
    {"github://", "github", OpenDALSecretType::GITHUB},
    {"hf://", "hf", OpenDALSecretType::HF},
    {"huggingface://", "huggingface", OpenDALSecretType::HUGGINGFACE},
    {"ipfs://", "ipfs", OpenDALSecretType::IPFS},
    {"koofr://", "koofr", OpenDALSecretType::KOOFR},
    {"lakefs://", "lakefs", OpenDALSecretType::LAKEFS},
    {"obs://", "obs", OpenDALSecretType::OBS},
    {"onedrive://", "onedrive", OpenDALSecretType::ONEDRIVE},
    {"oss://", "oss", OpenDALSecretType::OSS},
    {"pcloud://", "pcloud", OpenDALSecretType::PCLOUD},
    {"seafile://", "seafile", OpenDALSecretType::SEAFILE},
    {"sftp://", "sftp", OpenDALSecretType::SFTP},
    {"swift://", "swift", OpenDALSecretType::SWIFT},
    {"tos://", "tos", OpenDALSecretType::TOS},
    {"upyun://", "upyun", OpenDALSecretType::UPYUN},
    {"vercel-artifacts://", "vercel-artifacts", OpenDALSecretType::VERCEL_ARTIFACTS},
    {"vercel-blob://", "vercel-blob", OpenDALSecretType::VERCEL_BLOB},
    {"webdav://", "webdav", OpenDALSecretType::WEBDAV},
    {"yandex-disk://", "yandex-disk", OpenDALSecretType::YANDEX_DISK},
};

constexpr const char *OPENDAL_SECRET_TYPE_NAMES[] = {
    "",
    "http",
    "opendal_aliyun_drive",
    "opendal_azblob",
    "opendal_azdls",
    "opendal_azfile",
    "opendal_b2",
    "opendal_cos",
    "opendal_dbfs",
    "opendal_dropbox",
    "opendal_ftp",
    "opendal_gcs",
    "opendal_gdrive",
    "opendal_github",
    "opendal_hf",
    "opendal_huggingface",
    "opendal_ipfs",
    "opendal_koofr",
    "opendal_lakefs",
    "opendal_obs",
    "opendal_onedrive",
    "opendal_oss",
    "opendal_pcloud",
    "opendal_s3",
    "opendal_seafile",
    "opendal_sftp",
    "opendal_swift",
    "opendal_tos",
    "opendal_upyun",
    "opendal_vercel_artifacts",
    "opendal_vercel_blob",
    "opendal_webdav",
    "opendal_yandex_disk",
};

static_assert(sizeof(OPENDAL_SECRET_TYPE_NAMES) / sizeof(OPENDAL_SECRET_TYPE_NAMES[0]) ==
              static_cast<idx_t>(OpenDALSecretType::COUNT));

} // namespace

const char *OpenDALSecretTypeName(OpenDALSecretType type_p) {
	const auto index = static_cast<idx_t>(type_p);
	if (index >= static_cast<idx_t>(OpenDALSecretType::COUNT)) {
		return "";
	}
	return OPENDAL_SECRET_TYPE_NAMES[index];
}

bool OpenDALPath::TryParse(const string &path_p, OpenDALPath &result_p) {
	for (const auto &entry : OPENDAL_PREFIXES) {
		if (!StringUtil::StartsWith(path_p, string(entry.prefix))) {
			continue;
		}
		result_p.scheme.assign(entry.scheme.data(), entry.scheme.size());
		result_p.secret_type = entry.secret_type;
		result_p.uri_config.clear();
		const auto uri = OpenDALUri::Parse(path_p, entry.prefix.size());
		uri.Apply(result_p.scheme, string(entry.prefix), result_p.uri_config, result_p.path);
		return true;
	}
	return false;
}

} // namespace duckdb
