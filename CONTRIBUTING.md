# Contributing

## Did you find a bug?

* Search the [existing issues](https://github.com/dentiny/duckdb-opendal-filesystem/issues) first.
* If the bug has not been reported, [open an issue](https://github.com/dentiny/duckdb-opendal-filesystem/issues/new/choose) with a clear description, relevant environment details, and a minimal SQL or code reproduction.

## Did you write a patch that fixes a bug?

* Add a regression test whenever possible.
* Run the formatter with `make format-all`.
* Build with `CMAKE_BUILD_PARALLEL_LEVEL=12 make reldebug`.
* Run the focused test with `build/reldebug/test/unittest test/sql/duckdb_opendalfs.test`.
* Open a pull request and explain the problem, the solution, and any relevant issue.

## Pull requests

* Do not push directly to `main`; work in a branch and open a pull request.
* Keep branches current with `main` and prefer small, focused pull requests.
* Use issues or discussions for design work before starting a large change.
* The maintainers retain final discretion over whether a pull request is merged.

## Building

* Install `ccache` to improve compilation speed.
* Initialize dependencies with `git submodule update --init --recursive`.
* The OpenDAL C++ binding requires a Rust toolchain with Cargo.
* Build the project with `CMAKE_BUILD_PARALLEL_LEVEL=12 make reldebug`.

## Testing

Run the extension SQL test:

```sh
build/reldebug/test/unittest test/sql/duckdb_opendalfs.test
```

Run the full configured SQL test suite with `make test_reldebug`.

## Formatting

* Use tabs for indentation and spaces for alignment.
* Keep lines at or below 120 columns.
* Run `make format-all` before submitting a pull request.

### DuckDB C++ guidelines

* Prefer smart pointers; raw `new` and `delete` are code smells.
* Prefer `unique_ptr` over `shared_ptr` unless ownership is genuinely shared.
* Use `const` whenever possible and avoid importing namespaces such as `using namespace std`.
* Put extension code in the `duckdb` namespace and OpenDAL abstraction code in an appropriate project namespace.
* Use `override` or `final` for virtual method overrides.
* Use fixed-width integer types and DuckDB's `idx_t` for offsets, indices, and counts.
* Prefer references, and use const references for non-trivial input objects.
* Use range-based loops when possible and braces around conditionals and loops.
* Choose descriptive names, avoid unnamed magic numbers, return early, and do not submit commented-out code.

## Error handling

* Use exceptions for errors that terminate a query, not for expected control flow.
* Add tests that exercise error paths whenever practical.
* Use `D_ASSERT` for internal invariants. Assertions must never be reachable through invalid user input.

## Naming conventions

* Files: lowercase with underscores, for example `opendal_file_system.cpp`.
* Types: CamelCase beginning with an uppercase letter, for example `OpenDALFileHandle`.
* Variables: lowercase with underscores, for example `file_offset`.
* Functions: CamelCase beginning with an uppercase letter, for example `OpenFile`.

## Release process

The extension is intended for the [DuckDB community extensions](https://github.com/duckdb/community-extensions) ecosystem. Before release, verify that it builds and runs on all supported platforms using the Docker images in [extension-ci-tools](https://github.com/duckdb/extension-ci-tools/tree/main/docker).

For example, on Linux amd64 with musl:

```sh
git clone https://github.com/duckdb/extension-ci-tools.git
cd extension-ci-tools/docker/linux_amd64_musl
docker build -t duckdb-ci-linux-amd64-musl .
docker run -it duckdb-ci-linux-amd64-musl

# Inside the container:
git clone https://github.com/dentiny/duckdb-opendal-filesystem.git
cd duckdb-opendal-filesystem
git submodule update --init --recursive
CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) make reldebug
```

## Updating DuckDB

Use the repository skill in `.skills/duckdb-extension-upgrade/SKILL.md`. DuckDB and extension-ci-tools must be updated together; OpenDAL remains pinned unless the DuckDB upgrade requires a compatibility change.
