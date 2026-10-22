#include "opendal_secret.hpp"
#include "opendal_config.hpp"

#include "duckdb/common/string_util.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/main/secret/secret.hpp"

#include <algorithm>

namespace duckdb {
namespace {

constexpr std::string_view OPENDAL_SECRET_PREFIX = "opendal_";
constexpr std::string_view OPENDAL_SECRET_PROVIDER = "config";

constexpr std::string_view OPENDAL_SECRET_SERVICES[] = {
    "aliyun_drive", "azblob",  "azdls",      "azfile", "b2",       "cos",   "dbfs",
    "dropbox",      "ftp",     "gcs",        "gdrive", "github",   "hf",    "huggingface",
    "ipfs",         "koofr",   "lakefs",     "obs",    "onedrive", "oss",   "pcloud",
    "s3",           "seafile", "sftp",       "swift",  "tos",      "upyun", "vercel_artifacts",
    "vercel_blob",  "webdav",  "yandex_disk"};

constexpr std::string_view OPENDAL_SECRET_PARAMETERS[] = {
    "access_key_id", "secret_access_key", "session_token", "region",      "endpoint", "bucket",       "root",
    "token",         "username",          "password",      "key",         "secret",   "account_name", "account_key",
    "client_id",     "client_secret",     "tenant_id",     "private_key", "role_arn"};

vector<string> DefaultScopes(const string &service_p) {
	if (service_p == "s3") {
		return {"s3://", "s3a://", "s3n://"};
	}
	if (service_p == "gcs") {
		return {"gcs://", "gs://"};
	}
	if (service_p == "azblob") {
		return {"azblob://", "az://"};
	}
	if (service_p == "azdls") {
		return {"azdls://", "abfs://", "abfss://"};
	}
	auto prefix = service_p;
	std::replace(prefix.begin(), prefix.end(), '_', '-');
	return {prefix + "://"};
}

unique_ptr<BaseSecret> CreateOpenDALSecret(ClientContext &, CreateSecretInput &input_p) {
	const auto service = StringUtil::Lower(input_p.type.GetIdentifierName().substr(OPENDAL_SECRET_PREFIX.size()));
	auto scopes = input_p.scope.empty() ? DefaultScopes(service) : input_p.scope;
	auto secret = make_uniq<KeyValueSecret>(scopes, input_p.type, input_p.provider, input_p.name);
	for (const auto parameter : OPENDAL_SECRET_PARAMETERS) {
		secret->TrySetValue(Identifier(parameter), input_p);
	}
	secret->redact_keys = {"secret_access_key", "session_token", "token",      "password", "secret",
	                       "account_key",       "client_secret", "private_key"};
	return secret;
}

} // namespace

void RegisterOpenDALSecrets(ExtensionLoader &loader_p) {
	for (const auto service : OPENDAL_SECRET_SERVICES) {
		const string type = StringUtil::Format("%s%s", string(OPENDAL_SECRET_PREFIX), string(service));
		SecretType secret_type;
		secret_type.name = Identifier(type);
		secret_type.deserializer = KeyValueSecret::Deserialize<KeyValueSecret>;
		secret_type.default_provider = OPENDAL_SECRET_PROVIDER;
		secret_type.extension = OPENDALFS_EXTENSION_NAME;
		loader_p.RegisterSecretType(secret_type);

		CreateSecretFunction function = {type, Identifier(OPENDAL_SECRET_PROVIDER), CreateOpenDALSecret};
		for (const auto parameter : OPENDAL_SECRET_PARAMETERS) {
			function.named_parameters[Identifier(parameter)] = LogicalType::VARCHAR;
		}
		loader_p.RegisterFunction(function);
	}
}

} // namespace duckdb
