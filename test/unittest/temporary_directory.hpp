#pragma once

#include "duckdb/common/string.hpp"

#include <filesystem>

namespace duckdb {

class TemporaryDirectory {
public:
	TemporaryDirectory();
	~TemporaryDirectory();

	string Path() const;

private:
	std::filesystem::path path;
};

} // namespace duckdb
