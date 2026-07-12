PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# OpenDAL's C++ binding invokes Cargo from CMake-generated build rules. Keep a
# project-local toolchain discoverable by configure and every nested make.
export CARGO_HOME := $(PROJ_DIR).cargo
export RUSTUP_HOME := $(PROJ_DIR).rustup
export PATH := $(CARGO_HOME)/bin:$(PATH)
export MACOSX_DEPLOYMENT_TARGET ?= 15.0

# Configuration of extension
EXT_NAME=duckdb_opendalfs
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# Format extension sources/tests through the shared DuckDB formatter, then
# format this extension's standalone CMake project.
format-all: format
	cmake-format -i CMakeLists.txt
	cmake-format -i test/unittest/CMakeLists.txt

.PHONY: format-all

test_release_unit: release
	./build/release/extension/duckdb_opendalfs/test/unittest/unittest_duckdb_opendalfs

test_reldebug_unit: reldebug
	./build/reldebug/extension/duckdb_opendalfs/test/unittest/unittest_duckdb_opendalfs

test_debug_unit: debug
	./build/debug/extension/duckdb_opendalfs/test/unittest/unittest_duckdb_opendalfs

minio-up:
	docker compose -f test/minio/docker-compose.yml up -d --wait minio
	docker compose -f test/minio/docker-compose.yml run --rm minio-init

minio-down:
	docker compose -f test/minio/docker-compose.yml down -v

test_reldebug_minio: reldebug minio-up
	OPENDALFS_MINIO_AVAILABLE=1 ./build/reldebug/test/unittest test/sql/opendal_s3_minio.test

.PHONY: test_release_unit test_reldebug_unit test_debug_unit minio-up minio-down test_reldebug_minio
