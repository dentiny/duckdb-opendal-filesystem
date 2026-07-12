#pragma once

namespace duckdb {

struct OpenDALOpenOptions {
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
