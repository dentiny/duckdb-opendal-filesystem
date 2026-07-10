---
name: upgrade-duckdb-extension
description: Upgrade duckdb_opendalfs to a new DuckDB release. Use when the user asks to upgrade DuckDB, bump the duckdb submodule, sync to a new DuckDB tag, or update DuckDB and extension-ci-tools together.
---

# Upgrade duckdb_opendalfs to a new DuckDB release

DuckDB and extension-ci-tools must move together. OpenDAL is independently pinned and should only move when the target DuckDB version requires a compatibility fix. Then update CI metadata, build, format, and run the extension test.

## Inputs

Confirm the target DuckDB release tag, for example `v1.6.0`, before making changes.

## Workflow

Track these steps as a checklist and do not skip ahead:

1. Fetch tags and pin the `duckdb` submodule to `tags/$TARGET`.
2. Pin `extension-ci-tools` to the matching release tag or the release-compatible tag required by DuckDB.
3. Update DuckDB and CI-tools version references in `.github/workflows/MainDistributionPipeline.yml`.
4. Leave the `opendal` submodule unchanged unless compilation demonstrates a compatibility issue.
5. Run `make format-all`.
6. Build with `CMAKE_BUILD_PARALLEL_LEVEL=12 make reldebug`.
7. Run `build/reldebug/test/unittest test/sql/duckdb_opendalfs.test`.
8. Review `git diff --submodule=log` and record any compatibility changes in the pull request.

## Verification

The upgrade is complete only when:

* DuckDB and extension-ci-tools point at compatible release revisions.
* The static and loadable `duckdb_opendalfs` extension targets link successfully.
* The focused SQL test passes.
* CI metadata names the same DuckDB release.

## Current dependency model

* `duckdb`: pinned to a DuckDB release tag.
* `extension-ci-tools`: pinned to the compatible DuckDB extension tooling release.
* `opendal`: independently pinned to the C++ binding revision used by this extension.
