#include "opendal_open_options.hpp"

namespace duckdb {

OpenDALOpenOptions OpenDALOpenOptions::ReadOnly() {
	return {};
}

OpenDALOpenOptions OpenDALOpenOptions::WriteOnly(bool truncate_p) {
	OpenDALOpenOptions options;
	options.read = false;
	options.write = true;
	options.create = true;
	options.truncate = truncate_p;
	return options;
}

OpenDALOpenOptions OpenDALOpenOptions::ReadWrite(bool create_p) {
	OpenDALOpenOptions options;
	options.write = true;
	options.create = create_p;
	return options;
}

bool OpenDALOpenOptions::IsValid() const {
	if (!read && !write) {
		return false;
	}
	if ((create || truncate || append) && !write) {
		return false;
	}
	return true;
}

} // namespace duckdb
