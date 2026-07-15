# DuckDB OpenDAL Filesystem

`duckdb_opendalfs` is a DuckDB extension that makes remote storage services available through DuckDB's virtual
filesystem using [Apache OpenDAL](https://opendal.apache.org/).

## What is OpenDAL?

Apache OpenDAL is a data access layer that provides one consistent API over many storage services, including Amazon
S3-compatible storage, Google Cloud Storage, Azure Blob Storage, Hugging Face, WebDAV, SFTP, and others. OpenDAL owns
the backend-specific request, authentication, and object-storage behavior; this extension adapts that API to DuckDB's
filesystem interface.

## Why this extension?

DuckDB can query files directly through its virtual filesystem, but every storage integration otherwise needs its own
DuckDB filesystem implementation. This extension uses OpenDAL as the shared storage layer so that one DuckDB extension
can support many backends with consistent URI handling and secret management.

For example, after loading the extension, DuckDB can read remote CSV and Parquet files directly:

```sql
SELECT * FROM read_csv_auto('https://example.com/data.csv');
SELECT * FROM read_parquet('s3://analytics/events.parquet');
```

Paths with unknown schemes and local paths are not claimed by this extension; DuckDB can route those paths to another
registered filesystem.

## How to use extension

```sh
FORCE INSTALL opendalfs FROM community;
LOAD opendalfs;
```

## Secrets

Credentials and backend settings are configured with DuckDB secrets. Each OpenDAL backend has a secret type named
`opendal_<service>`, such as `opendal_s3`, `opendal_gcs`, or `opendal_hf`. The extension selects the best matching
secret using the file URI and the secret's scope.

### S3

```sql
CREATE SECRET production_s3 (
    TYPE opendal_s3,
    SCOPE 's3://analytics',
    ACCESS_KEY_ID 'access-key',
    SECRET_ACCESS_KEY 'secret-key',
    REGION 'us-east-1'
);

SELECT * FROM read_parquet('s3://analytics/events.parquet');
```

The default scope for `opendal_s3` covers `s3://`, `s3a://`, and `s3n://`. An explicit scope is recommended when
different buckets or prefixes require different credentials.

For an S3-compatible service such as MinIO, provide its endpoint:

```sql
CREATE SECRET local_minio (
    TYPE opendal_s3,
    SCOPE 's3://test',
    ACCESS_KEY_ID 'minioadmin',
    SECRET_ACCESS_KEY 'minioadmin',
    REGION 'us-east-1',
    ENDPOINT 'http://127.0.0.1:19000'
);

COPY (SELECT 42 AS value) TO 's3://test/example.csv' (HEADER);
SELECT * FROM read_csv_auto('s3://test/example.csv');
```

### Google Cloud Storage

```sql
CREATE SECRET production_gcs (
    TYPE opendal_gcs,
    SCOPE 'gcs://analytics',
    TOKEN 'access-token'
);

SELECT * FROM read_parquet('gcs://analytics/events.parquet');
```

The `opendal_gcs` default scope covers both `gcs://` and `gs://`.

### Secret parameters

The extension currently accepts these common OpenDAL configuration fields:

```text
access_key_id, secret_access_key, session_token, region, endpoint, bucket, root,
token, username, password, key, secret, account_name, account_key, client_id,
client_secret, tenant_id, private_key, role_arn
```

Sensitive fields are redacted by DuckDB when secrets are displayed. DuckDB's normal secret rules apply, including
temporary versus persistent secrets and longest-prefix scope matching. See the
[DuckDB secrets documentation](https://duckdb.org/docs/stable/configuration/secrets_manager) for secret lifecycle and
storage behavior.

## Known limitation

- Every read operation currently incurs an extra memory copy because DuckDB's filesystem read API uses caller-provided
  memory buffers. See [DuckDB discussion #21546](https://github.com/duckdb/duckdb/discussions/21546) for details.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
