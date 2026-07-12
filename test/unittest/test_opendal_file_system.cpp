#include "catch/catch.hpp"
#include "opendal_file_system.hpp"

#include <opendal.hpp>

#include <cstring>

namespace duckdb {

TEST_CASE("OpenDAL filesystem handles registered prefixes only", "[opendalfs]") {
	OpenDALFileSystem fs;

	REQUIRE(fs.CanSeek());
	REQUIRE(!fs.IsLocalFileSystem());
	REQUIRE(fs.GetName() == "duckdb_opendalfs");
	REQUIRE(fs.PathSeparator("memory://hello.txt") == "/");
	REQUIRE(fs.IsPathAbsolute("memory://hello.txt"));
	REQUIRE(!fs.IsPathAbsolute("hello.txt"));
	REQUIRE(!fs.IsPathAbsolute("unknown://hello.txt"));
	REQUIRE(fs.CanHandleFile("memory://hello.txt"));
	REQUIRE(fs.CanHandleFile("s3://bucket/key"));
	REQUIRE(fs.CanHandleFile("https://example.com/file"));
	REQUIRE(!fs.CanHandleFile("hello.txt"));
	REQUIRE(!fs.CanHandleFile("/tmp/hello.txt"));
	REQUIRE(!fs.CanHandleFile("file:///tmp/hello.txt"));
	REQUIRE(!fs.CanHandleFile("unknown://hello.txt"));
}

TEST_CASE("OpenDAL filesystem writes, gets file size, and reads from the memory backend", "[opendalfs]") {
	OpenDALFileSystem fs;
	auto writer =
	    fs.OpenFile("memory://hello.txt", FileFlags::FILE_FLAGS_WRITE | FileFlags::FILE_FLAGS_FILE_CREATE_NEW);
	REQUIRE(!fs.OnDiskFile(*writer));
	string hello = "hello";
	REQUIRE(writer->Write(hello.data(), hello.size()) == hello.size());
	string suffix = " world";
	REQUIRE(writer->Write(suffix.data(), suffix.size()) == suffix.size());
	REQUIRE(writer->SeekPosition() == 11);
	REQUIRE(writer->GetFileSize() == 11);
	fs.Truncate(*writer, 5);
	REQUIRE(writer->SeekPosition() == 5);
	REQUIRE(writer->GetFileSize() == 5);
	REQUIRE(writer->Write(suffix.data(), suffix.size()) == suffix.size());
	REQUIRE(writer->GetFileSize() == 11);
	fs.FileSync(*writer);
	writer->Close();
	writer->Close();

	auto op = make_uniq<opendal::Operator>("memory");
	op->Write("hello.txt", "hello world");
	OpenDALFileHandle reader(fs, std::move(op), "memory://hello.txt", "hello.txt", FileFlags::FILE_FLAGS_READ,
	                         OpenDALOpenOptions::ReadOnly());
	REQUIRE(reader.Size() == 11);

	char buffer[16] = {};
	REQUIRE(reader.Read(buffer, 5) == 5);
	REQUIRE(string(buffer, 5) == "hello");
	REQUIRE(fs.SeekPosition(reader) == 5);

	std::memset(buffer, 0, sizeof(buffer));
	fs.Seek(reader, 6);
	REQUIRE(reader.Read(buffer, 5) == 5);
	REQUIRE(string(buffer, 5) == "world");
	REQUIRE(fs.SeekPosition(reader) == 11);
	fs.Reset(reader);
	REQUIRE(fs.SeekPosition(reader) == 0);
	std::memset(buffer, 0, sizeof(buffer));
	REQUIRE(reader.Read(buffer, 5) == 5);
	REQUIRE(string(buffer, 5) == "hello");
	reader.Close();
}

TEST_CASE("OpenDAL filesystem rejects invalid paths and closed handles", "[opendalfs]") {
	OpenDALFileSystem fs;
	REQUIRE_THROWS(fs.OpenFile("missing.txt", FileFlags::FILE_FLAGS_READ));
	REQUIRE_THROWS(fs.OpenFile("unknown://missing.txt", FileFlags::FILE_FLAGS_READ));
	REQUIRE(!fs.FileExists("missing.txt"));
	REQUIRE(!fs.FileExists("unknown://missing.txt"));

	auto writer =
	    fs.OpenFile("memory://closed.txt", FileFlags::FILE_FLAGS_WRITE | FileFlags::FILE_FLAGS_FILE_CREATE_NEW);
	char unflushed[] = "unflushed";
	writer->Write(unflushed, 9);
	writer->Close();
	char value[] = "x";
	REQUIRE_THROWS(writer->Write(value, 1));
}

TEST_CASE("OpenDAL filesystem removes objects from the memory backend", "[opendalfs]") {
	OpenDALFileSystem fs;

	REQUIRE_NOTHROW(fs.RemoveFile("memory://removed.txt"));
	REQUIRE(fs.TryRemoveFile("memory://removed.txt"));
	REQUIRE(!fs.TryRemoveFile("removed.txt"));
	REQUIRE(!fs.TryRemoveFile("unknown://removed.txt"));
	REQUIRE(!fs.TryRemoveFile("memory://"));
	REQUIRE_NOTHROW(fs.RemoveFiles({"memory://first.txt", "memory://second.txt"}));
	REQUIRE_THROWS(fs.RemoveFiles({"memory://first.txt", "second.txt"}));
	REQUIRE_THROWS(fs.RemoveFile("removed.txt"));
	REQUIRE_THROWS(fs.RemoveFile("unknown://removed.txt"));
	REQUIRE_THROWS(fs.RemoveFile("memory://"));
}

TEST_CASE("OpenDAL filesystem creates and lists directories in the memory backend", "[opendalfs]") {
	OpenDALFileSystem fs;

	REQUIRE_NOTHROW(fs.CreateDirectory("memory://directory/"));
	REQUIRE_NOTHROW(fs.CreateDirectory("memory://directory/first/"));
	REQUIRE_NOTHROW(fs.CreateDirectory("memory://directory/second/"));
	idx_t entry_count = 0;
	REQUIRE(fs.ListFiles("memory://directory/", [&](const string &, bool) { entry_count++; }));
	REQUIRE(entry_count == 0);
	REQUIRE(!fs.DirectoryExists("directory/"));
	REQUIRE(!fs.DirectoryExists("unknown://directory/"));
	REQUIRE(!fs.DirectoryExists("memory://"));
	REQUIRE_NOTHROW(fs.RemoveDirectory("memory://directory/first/"));
	REQUIRE_NOTHROW(fs.RemoveDirectory("memory://directory/second/"));
	REQUIRE_NOTHROW(fs.RemoveDirectory("memory://directory/"));
	REQUIRE_THROWS(fs.CreateDirectory("directory/"));
	REQUIRE_THROWS(fs.CreateDirectory("unknown://directory/"));
	REQUIRE_THROWS(fs.CreateDirectory("memory://"));
	REQUIRE_THROWS(fs.ListFiles("directory/", [](const string &, bool) {}));
	REQUIRE_THROWS(fs.ListFiles("unknown://directory/", [](const string &, bool) {}));
	REQUIRE_THROWS(fs.ListFiles("memory://", [](const string &, bool) {}));
	REQUIRE_THROWS(fs.RemoveDirectory("directory/"));
	REQUIRE_THROWS(fs.RemoveDirectory("unknown://directory/"));
	REQUIRE_THROWS(fs.RemoveDirectory("memory://"));
}

TEST_CASE("OpenDAL filesystem validates move paths", "[opendalfs]") {
	OpenDALFileSystem fs;

	REQUIRE_THROWS(fs.MoveFile("source.txt", "memory://target.txt"));
	REQUIRE_THROWS(fs.MoveFile("memory://source.txt", "target.txt"));
	REQUIRE_THROWS(fs.MoveFile("memory://", "memory://target.txt"));
	REQUIRE_THROWS(fs.MoveFile("memory://source.txt", "memory://"));
}

} // namespace duckdb
