#include "catch/catch.hpp"
#include "opendal_file_system.hpp"

#include <cstring>
#include <filesystem>

namespace duckdb {

class TestFileSystem {
public:
	TestFileSystem() : root(PrepareRoot()), fs("fs", {{"root", root.string()}}) {
	}

	~TestFileSystem() {
		std::error_code error;
		std::filesystem::remove_all(root, error);
	}

private:
	static std::filesystem::path PrepareRoot() {
		auto root = std::filesystem::temp_directory_path() / "duckdb_opendalfs_unit_test";
		std::filesystem::remove_all(root);
		std::filesystem::create_directories(root);
		return root;
	}

public:
	std::filesystem::path root;
	OpenDALFileSystem fs;
};

TEST_CASE("OpenDAL filesystem opens, writes, reads, and closes", "[opendalfs]") {
	TestFileSystem test_fs;
	auto &fs = test_fs.fs;

	auto writer = fs.Open("hello.txt", OpenDALOpenOptions::WriteOnly());
	const string hello = "hello";
	REQUIRE(writer->Write(hello.data(), hello.size()) == hello.size());
	const string suffix = " world";
	REQUIRE(writer->Write(suffix.data(), suffix.size()) == suffix.size());
	REQUIRE(writer->Tell() == 11);
	writer->Flush();
	writer->Close();
	writer->Close();

	auto reader = fs.Open("hello.txt", OpenDALOpenOptions::ReadOnly());
	char buffer[16] = {};
	REQUIRE(reader->Read(buffer, 5) == 5);
	REQUIRE(string(buffer, 5) == "hello");
	REQUIRE(reader->Tell() == 5);

	std::memset(buffer, 0, sizeof(buffer));
	REQUIRE(reader->Read(buffer, 5, 6) == 5);
	REQUIRE(string(buffer, 5) == "world");
	REQUIRE(reader->Tell() == 5);
	reader->Close();
}

TEST_CASE("OpenDAL filesystem supports read-write and truncate", "[opendalfs]") {
	TestFileSystem test_fs;
	auto &fs = test_fs.fs;
	{
		auto writer = fs.Open("edit.txt", OpenDALOpenOptions::WriteOnly());
		const string initial = "abcdef";
		writer->Write(initial.data(), initial.size());
		writer->Flush();
	}

	auto handle = fs.Open("edit.txt", OpenDALOpenOptions::ReadWrite());
	const string replacement = "XY";
	handle->Write(replacement.data(), replacement.size(), 2);
	char buffer[8] = {};
	REQUIRE(handle->Read(buffer, /*size=*/6, /*offset=*/0) == 6);
	REQUIRE(string(buffer, 6) == "abXYef");
	handle->Truncate(4);
	handle->Flush();
	handle->Close();

	auto reader = fs.Open("edit.txt", OpenDALOpenOptions::ReadOnly());
	std::memset(buffer, 0, sizeof(buffer));
	REQUIRE(reader->Read(buffer, sizeof(buffer)) == 4);
	REQUIRE(string(buffer, 4) == "abXY");
}

TEST_CASE("OpenDAL filesystem rejects invalid handle access", "[opendalfs]") {
	TestFileSystem test_fs;
	auto &fs = test_fs.fs;
	REQUIRE_THROWS(fs.Open("missing.txt", OpenDALOpenOptions::ReadOnly()));

	auto writer = fs.Open("closed.txt", OpenDALOpenOptions::WriteOnly());
	writer->Write("unflushed", 9);
	writer->Close();
	REQUIRE(!fs.Exists("closed.txt"));
	REQUIRE_THROWS(writer->Write("x", 1));
}

} // namespace duckdb
