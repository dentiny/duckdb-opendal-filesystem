# Changelog

## 0.3.0

### Changed

- Upgrade DuckDB to `v2.0.0` and extension-ci-tools to the latest commit.
- Require C++17 throughout the extension and publish the `0.3.0` release metadata.
- Use constexpr string views for global setting, description, service, and secret-name constants.
- Adapt configuration and secret registration to DuckDB's Identifier APIs.
- Reuse DuckDB metadata version tags and secret access helpers.
- Pass OpenDAL's native option container directly, removing ownership-conversion adapters.
- Update static registration and prebuilt-library unit-test linkage for DuckDB v2.

## 0.2.1

### Added

- Implement batch deletion ([#45])

[#45]: https://github.com/dentiny/duckdb-opendal-filesystem/pull/45

### Updated

- Update DuckDB and extension-ci-tools to `v1.5.6`.

- Update OpenDAL ([#45])

[#45]: https://github.com/dentiny/duckdb-opendal-filesystem/pull/45

## 0.2.0

### Added

- Add retry and timeout layer ([#38])

[#38]: https://github.com/dentiny/duckdb-opendal-filesystem/pull/38

### Fixed

- Integrate with OpenDAL streaming writer instead of blocking writer ([#39])

[#39]: https://github.com/dentiny/duckdb-opendal-filesystem/pull/39

## 0.1.1

### Changed

- Bump all submodules to DuckDB `v1.5.5`.

- Pin OpenDAL to the latest upstream `main` commit.
