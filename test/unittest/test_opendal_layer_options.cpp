#include "catch/catch.hpp"
#include "duckdb/common/exception.hpp"
#include "opendal_layer_options.hpp"

namespace duckdb {

TEST_CASE("OpenDAL layer options preserve pinned defaults", "[opendalfs]") {
	OpenDALLayerOptions options;
	REQUIRE_NOTHROW(options.Validate());

	auto operator_options = options.ToOperatorOptions();
	REQUIRE(operator_options.timeout_ms == 60000);
	REQUIRE(operator_options.io_timeout_ms == 10000);
	REQUIRE(operator_options.retry_max_times == 3);
	REQUIRE(operator_options.retry_factor == 2.0F);
	REQUIRE(operator_options.retry_min_delay_ms == 1000);
	REQUIRE(operator_options.retry_max_delay_ms == 60000);
	REQUIRE(!operator_options.retry_jitter);

	// This constructor exercises explicit timeout and retry layer composition.
	opendal::Operator op("memory", {}, operator_options);
	op.Write("layers.txt", "configured");
	REQUIRE(op.Read("layers.txt") == "configured");
}

TEST_CASE("OpenDAL layer options reject invalid values", "[opendalfs]") {
	OpenDALLayerOptions options;
	options.timeout_ms = 0;
	REQUIRE_THROWS_AS(options.Validate(), InvalidInputException);

	options = OpenDALLayerOptions();
	options.io_timeout_ms = 0;
	REQUIRE_THROWS_AS(options.Validate(), InvalidInputException);

	options = OpenDALLayerOptions();
	options.retry_factor = 0.5;
	REQUIRE_THROWS_AS(options.Validate(), InvalidInputException);

	options = OpenDALLayerOptions();
	options.retry_min_delay_ms = 11;
	options.retry_max_delay_ms = 10;
	REQUIRE_THROWS_AS(options.Validate(), InvalidInputException);
}

} // namespace duckdb
