#include "opendal_open_options.hpp"

namespace duckdb {

OpenDALOpenOptions::OpenDALOpenOptions(FileOpenFlags flags_p)
    : read(flags_p.OpenForReading()), write(flags_p.OpenForWriting()),
      create(flags_p.CreateFileIfNotExists() || flags_p.OverwriteExistingFile()),
      truncate(flags_p.OverwriteExistingFile()), append(flags_p.OpenForAppending()) {
}

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
