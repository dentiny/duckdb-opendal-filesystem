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

.PHONY: format-all
