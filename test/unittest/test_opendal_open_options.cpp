#include "catch/catch.hpp"
#include "opendal_open_options.hpp"

namespace duckdb {

TEST_CASE("OpenDAL open options validate access modes", "[opendalfs]") {
	REQUIRE(OpenDALOpenOptions::ReadOnly().IsValid());
	REQUIRE(OpenDALOpenOptions::WriteOnly().IsValid());
	REQUIRE(OpenDALOpenOptions::ReadWrite().IsValid());

	OpenDALOpenOptions options;
	options.read = false;
	REQUIRE(!options.IsValid());

	options.create = true;
	REQUIRE(!options.IsValid());

	options.create = false;
	options.truncate = true;
	REQUIRE(!options.IsValid());

	options.truncate = false;
	options.append = true;
	REQUIRE(!options.IsValid());
}

} // namespace duckdb
