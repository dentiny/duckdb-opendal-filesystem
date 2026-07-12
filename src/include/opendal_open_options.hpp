#pragma once

#include "duckdb/common/file_open_flags.hpp"

namespace duckdb {

struct OpenDALOpenOptions {
	OpenDALOpenOptions() = default;
	explicit OpenDALOpenOptions(FileOpenFlags flags_p);

	bool read = true;
	bool write = false;
	bool create = false;
	bool truncate = false;
	bool append = false;

	static OpenDALOpenOptions ReadOnly();
	static OpenDALOpenOptions WriteOnly();

	bool IsValid() const;
};

} // namespace duckdb
