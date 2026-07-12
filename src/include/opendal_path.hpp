#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/unordered_map.hpp"

namespace duckdb {

enum class OpenDALSecretType : uint8_t {
	NONE,
	HTTP,
	ALIYUN_DRIVE,
	AZBLOB,
	AZDLS,
	AZFILE,
	B2,
	COS,
	DBFS,
	DROPBOX,
	FTP,
	GCS,
	GDRIVE,
	GITHUB,
	HF,
	HUGGINGFACE,
	IPFS,
	KOOFR,
	LAKEFS,
	OBS,
	ONEDRIVE,
	OSS,
	PCLOUD,
	S3,
	SEAFILE,
	SFTP,
	SWIFT,
	TOS,
	UPYUN,
	VERCEL_ARTIFACTS,
	VERCEL_BLOB,
	WEBDAV,
	YANDEX_DISK,
	COUNT
};

const char *OpenDALSecretTypeName(OpenDALSecretType type_p);

struct OpenDALPath {
	string scheme;
	OpenDALSecretType secret_type = OpenDALSecretType::NONE;
	string path;
	unordered_map<string, string> uri_config;

	static bool TryParse(const string &path_p, OpenDALPath &result_p);
};

} // namespace duckdb
