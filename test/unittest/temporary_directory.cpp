#include "temporary_directory.hpp"

#include <chrono>
#include <system_error>

namespace duckdb {

TemporaryDirectory::TemporaryDirectory() {
	auto name = "duckdb-opendalfs-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
	path = std::filesystem::temp_directory_path() / name;
	std::filesystem::create_directories(path);
}

TemporaryDirectory::~TemporaryDirectory() {
	std::error_code error;
	std::filesystem::remove_all(path, error);
}

string TemporaryDirectory::Path() const {
	return path.string();
}

} // namespace duckdb
