cmake_minimum_required(VERSION 3.22)

# Set extension name here
set(TARGET_NAME duckdb_opendalfs)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})

set(CMAKE_CXX_STANDARD
    "17"
    CACHE STRING "C++ standard to enforce")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src/include)

# Build the synchronous OpenDAL C++ binding from the pinned submodule. The
# binding drives its Rust build through Cargo and exposes the opendal_cpp CMake
# target, including all generated cxx bridge headers and native libraries.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.cargo/bin/cargo")
  set(ENV{CARGO_HOME} "${CMAKE_CURRENT_LIST_DIR}/.cargo")
  set(ENV{RUSTUP_HOME} "${CMAKE_CURRENT_LIST_DIR}/.rustup")
  set(ENV{PATH} "${CMAKE_CURRENT_LIST_DIR}/.cargo/bin:$ENV{PATH}")
endif()
set(OPENDAL_ENABLE_ASYNC
    OFF
    CACHE BOOL "Build OpenDAL's async C++ API" FORCE)
set(OPENDAL_ENABLE_TESTING
    OFF
    CACHE BOOL "Build OpenDAL's C++ tests" FORCE)
set(OPENDAL_ENABLE_DOCUMENTATION
    OFF
    CACHE BOOL "Build OpenDAL's C++ docs" FORCE)
# OpenDAL registers services at compile time. Enable every storage backend
# exposed by the pinned OpenDAL revision so one extension binary can select any
# of them at runtime.
set(OPENDAL_SERVICE_FEATURES
    services-aliyun-drive
    services-azblob
    services-azdls
    services-azfile
    services-b2
    services-compfs
    services-cos
    services-dbfs
    services-dropbox
    services-ftp
    services-gcs
    services-gdrive
    services-github
    services-hf
    services-http
    services-huggingface
    services-ipfs
    services-ipmfs
    services-koofr
    services-lakefs
    services-memory
    services-obs
    services-onedrive
    services-oss
    services-pcloud
    services-s3
    services-seafile
    services-sftp
    services-swift
    services-tos
    services-upyun
    services-vercel-artifacts
    services-vercel-blob
    services-webdav
    services-yandex-disk)
list(TRANSFORM OPENDAL_SERVICE_FEATURES PREPEND "opendal/")
list(JOIN OPENDAL_SERVICE_FEATURES "," OPENDAL_FEATURES_VALUE)
set(OPENDAL_FEATURES
    "${OPENDAL_FEATURES_VALUE}"
    CACHE STRING "OpenDAL storage backend features" FORCE)
add_subdirectory(opendal/bindings/cpp opendal-cpp)

add_library(
  duckdb_opendalfs_core STATIC
  src/opendal_file_system.cpp src/opendal_open_options.cpp src/opendal_path.cpp)
target_compile_features(duckdb_opendalfs_core PUBLIC cxx_std_17)
target_include_directories(duckdb_opendalfs_core PUBLIC src/include)
target_link_libraries(duckdb_opendalfs_core PUBLIC opendal_cpp)

set(EXTENSION_SOURCES src/duckdb_opendalfs_extension.cpp)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

# OpenDAL's public C++ headers use std::optional and require C++17. DuckDB may
# establish an older default before loading an extension's CMake file, so set
# the requirement on the actual extension targets as well.
target_compile_features(${EXTENSION_NAME} PRIVATE cxx_std_17)
target_compile_features(${LOADABLE_EXTENSION_NAME} PRIVATE cxx_std_17)

# Both extension variants must carry OpenDAL: DuckDB's static extension build
# and the separately loadable .duckdb_extension artifact.
target_link_libraries(${EXTENSION_NAME} duckdb_opendalfs_core)
target_link_libraries(${LOADABLE_EXTENSION_NAME} duckdb_opendalfs_core)
if(APPLE)
  # Cargo's security-framework crates are embedded in the Rust static archive;
  # their native dependencies must be forwarded to every final consumer.
  target_link_libraries(
    opendal_cpp PUBLIC "-framework CoreFoundation" "-framework IOKit"
                       "-framework Security" "-framework SystemConfiguration")
endif()

install(
  TARGETS ${EXTENSION_NAME} duckdb_opendalfs_core opendal_cpp
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

add_subdirectory(test/unittest)
