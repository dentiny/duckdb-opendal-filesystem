#include "catch/catch.hpp"
#include "opendal_file_system.hpp"

#include <opendal.hpp>

#include <cstring>

namespace duckdb {

TEST_CASE("OpenDAL filesystem handles registered prefixes only", "[opendalfs]") {
	OpenDALFileSystem fs;

	REQUIRE(fs.CanHandleFile("memory://hello.txt"));
	REQUIRE(fs.CanHandleFile("s3://bucket/key"));
	REQUIRE(fs.CanHandleFile("https://example.com/file"));
	REQUIRE(!fs.CanHandleFile("hello.txt"));
	REQUIRE(!fs.CanHandleFile("/tmp/hello.txt"));
	REQUIRE(!fs.CanHandleFile("file:///tmp/hello.txt"));
	REQUIRE(!fs.CanHandleFile("unknown://hello.txt"));
}

TEST_CASE("OpenDAL filesystem writes to the memory backend", "[opendalfs]") {
	OpenDALFileSystem fs;
	auto writer = fs.Open("memory://hello.txt", OpenDALOpenOptions::WriteOnly());
	const string hello = "hello";
	REQUIRE(writer->Write(hello.data(), hello.size()) == hello.size());
	const string suffix = " world";
	REQUIRE(writer->Write(suffix.data(), suffix.size()) == suffix.size());
	REQUIRE(writer->Tell() == 11);
	REQUIRE(writer->Size() == 11);
	writer->Flush();
	writer->Close();
	writer->Close();
}

TEST_CASE("OpenDAL file handle reads from the memory backend", "[opendalfs]") {
	auto op = make_uniq<opendal::Operator>("memory");
	op->Write("hello.txt", "hello world");
	OpenDALFileHandle reader(std::move(op), "hello.txt", OpenDALOpenOptions::ReadOnly());

	char buffer[16] = {};
	REQUIRE(reader.Read(buffer, 5) == 5);
	REQUIRE(string(buffer, 5) == "hello");
	REQUIRE(reader.Tell() == 5);

	std::memset(buffer, 0, sizeof(buffer));
	REQUIRE(reader.Read(buffer, 5, 6) == 5);
	REQUIRE(string(buffer, 5) == "world");
	REQUIRE(reader.Tell() == 5);
	reader.Close();
}

TEST_CASE("OpenDAL filesystem rejects invalid paths and closed handles", "[opendalfs]") {
	OpenDALFileSystem fs;
	REQUIRE_THROWS(fs.Open("missing.txt", OpenDALOpenOptions::ReadOnly()));
	REQUIRE_THROWS(fs.Open("unknown://missing.txt", OpenDALOpenOptions::ReadOnly()));
	REQUIRE(!fs.Exists("missing.txt"));

	auto writer = fs.Open("memory://closed.txt", OpenDALOpenOptions::WriteOnly());
	writer->Write("unflushed", 9);
	writer->Close();
	REQUIRE_THROWS(writer->Write("x", 1));
}

} // namespace duckdb
