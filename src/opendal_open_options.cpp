#include "opendal_open_options.hpp"

namespace duckdb {

OpenDALOpenOptions OpenDALOpenOptions::ReadOnly() {
	return {};
}

OpenDALOpenOptions OpenDALOpenOptions::WriteOnly() {
	OpenDALOpenOptions options;
	options.read = false;
	options.write = true;
	options.create = true;
	options.truncate = true;
	return options;
}

bool OpenDALOpenOptions::IsValid() const {
	if (!read && !write) {
		return false;
	}
	if (read && write) {
		return false;
	}
	if ((create || truncate || append) && !write) {
		return false;
	}
	if (append) {
		return false;
	}
	return true;
}

} // namespace duckdb
