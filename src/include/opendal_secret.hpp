#pragma once

namespace duckdb {
class ExtensionLoader;
void RegisterOpenDALSecrets(ExtensionLoader &loader_p);
} // namespace duckdb
