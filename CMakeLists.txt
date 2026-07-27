cmake_minimum_required(VERSION 3.22)

# Set extension name here
set(TARGET_NAME duckdb_opendalfs)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})

include_directories(src/include)

# Build the synchronous OpenDAL C++ binding from the pinned submodule.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.cargo/bin/cargo")
  set(ENV{CARGO_HOME} "${CMAKE_CURRENT_LIST_DIR}/.cargo")
  set(ENV{RUSTUP_HOME} "${CMAKE_CURRENT_LIST_DIR}/.rustup")
  set(ENV{PATH} "${CMAKE_CURRENT_LIST_DIR}/.cargo/bin:$ENV{PATH}")
endif()
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
set(OPENDAL_ENABLE_ASYNC
    OFF
    CACHE BOOL "Build OpenDAL's async C++ API" FORCE)
set(OPENDAL_ENABLE_TESTING
    OFF
    CACHE BOOL "Build OpenDAL's C++ tests" FORCE)
set(OPENDAL_ENABLE_DOCUMENTATION
    OFF
    CACHE BOOL "Build OpenDAL's C++ docs" FORCE)
set(OPENDAL_FEATURES
    "${OPENDAL_FEATURES_VALUE}"
    CACHE STRING "OpenDAL storage backend features" FORCE)
add_subdirectory(opendal/bindings/cpp opendal-cpp)

if(APPLE AND Rust_CARGO_TARGET)
  set(OPENDAL_CARGO_TARGET_ROOT
      "${CMAKE_CURRENT_LIST_DIR}/opendal/bindings/cpp/target")
  add_custom_target(
    opendal_cpp_target_cxxbridge
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OPENDAL_CARGO_TARGET_ROOT}/cxxbridge"
    COMMAND
      ${CMAKE_COMMAND} -E create_symlink
      "${OPENDAL_CARGO_TARGET_ROOT}/${Rust_CARGO_TARGET}/cxxbridge"
      "${OPENDAL_CARGO_TARGET_ROOT}/cxxbridge"
    DEPENDS cargo_build)
  add_dependencies(opendal_cpp opendal_cpp_target_cxxbridge)
endif()

# OpenDAL installs its default HTTP transporter from a Rust static constructor.
# Retain the Rust archive published by the binding so that constructor is not
# discarded by the final executable or extension link.
if(APPLE OR (UNIX AND NOT WIN32))
  get_target_property(OPENDAL_CPP_INTERFACE_LIBS opendal_cpp
                      INTERFACE_LINK_LIBRARIES)
  list(GET OPENDAL_CPP_INTERFACE_LIBS 0 OPENDAL_RUST_LIB)
  list(REMOVE_ITEM OPENDAL_CPP_INTERFACE_LIBS "${OPENDAL_RUST_LIB}")
  set_target_properties(opendal_cpp PROPERTIES INTERFACE_LINK_LIBRARIES
                                               "${OPENDAL_CPP_INTERFACE_LIBS}")
  if(APPLE AND Rust_CARGO_TARGET)
    get_filename_component(OPENDAL_RUST_TARGET_DIR "${OPENDAL_RUST_LIB}"
                           DIRECTORY)
    get_filename_component(OPENDAL_RUST_TARGET_DIR "${OPENDAL_RUST_TARGET_DIR}"
                           DIRECTORY)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(OPENDAL_RUST_PROFILE debug)
    else()
      set(OPENDAL_RUST_PROFILE release)
    endif()
    set(OPENDAL_RUST_LIB
        "${OPENDAL_RUST_TARGET_DIR}/${Rust_CARGO_TARGET}/${OPENDAL_RUST_PROFILE}/${CMAKE_STATIC_LIBRARY_PREFIX}opendal_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
  endif()
  if(APPLE)
    target_link_libraries(opendal_cpp
                          INTERFACE "-Wl,-force_load,${OPENDAL_RUST_LIB}")
  else()
    target_link_libraries(
      opendal_cpp INTERFACE "-Wl,--whole-archive" "${OPENDAL_RUST_LIB}"
                            "-Wl,--no-whole-archive")
  endif()
endif()

add_library(
  duckdb_opendalfs_core STATIC
  src/opendal_file_system.cpp src/opendal_layer_options.cpp
  src/opendal_open_options.cpp src/opendal_path.cpp src/opendal_secret.cpp
  src/opendal_uri.cpp)
target_compile_features(duckdb_opendalfs_core PRIVATE cxx_std_17)
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
if(MINGW OR WIN32)
  target_link_libraries(${EXTENSION_NAME} userenv ws2_32 bcrypt)
  target_link_libraries(${LOADABLE_EXTENSION_NAME} userenv ws2_32 bcrypt)
endif()
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
