include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${DuckDB_SOURCE_DIR}/third_party)
include_directories(${DuckDB_SOURCE_DIR}/test/include)

set(OPENDALFS_UNITTEST_OBJECTS main.cpp test_opendal_file_system.cpp)

add_executable(unittest_duckdb_opendalfs ${OPENDALFS_UNITTEST_OBJECTS})
target_compile_features(unittest_duckdb_opendalfs PRIVATE cxx_std_17)

if(NOT WIN32
   AND NOT SUN
   AND NOT ZOS)
  target_link_libraries(unittest_duckdb_opendalfs test_helpers duckdb
                        ${EXTENSION_NAME})
else()
  target_link_libraries(unittest_duckdb_opendalfs test_helpers duckdb_static)
  link_extension_libraries(unittest_duckdb_opendalfs "")
endif()
