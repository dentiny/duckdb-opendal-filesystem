#include "catch/catch.hpp"
#include "opendal_file_system.hpp"
#include "opendal_path.hpp"

#include <opendal.hpp>

#include <cstring>
#include <thread>

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

TEST_CASE("OpenDAL paths configure every registered storage backend", "[opendalfs]") {
	struct TestCase {
		const char *uri;
		const char *scheme;
		const char *path;
		const char *config_key;
		const char *config_value;
	};
	const TestCase cases[] = {
	    {"memory://file", "memory", "file", "", ""},
	    {"s3://bucket/file", "s3", "file", "bucket", "bucket"},
	    {"s3a://bucket/file", "s3", "file", "bucket", "bucket"},
	    {"s3n://bucket/file", "s3", "file", "bucket", "bucket"},
	    {"gs://bucket/file", "gcs", "file", "bucket", "bucket"},
	    {"gcs://bucket/file", "gcs", "file", "bucket", "bucket"},
	    {"az://container/file", "azblob", "file", "container", "container"},
	    {"azblob://container/file", "azblob", "file", "container", "container"},
	    {"abfs://account.dfs.core.windows.net/filesystem/file", "azdls", "file", "filesystem", "filesystem"},
	    {"abfss://account.dfs.core.windows.net/filesystem/file", "azdls", "file", "filesystem", "filesystem"},
	    {"azdls://account.dfs.core.windows.net/filesystem/file", "azdls", "file", "filesystem", "filesystem"},
	    {"azfile://account.file.core.windows.net/share/file", "azfile", "file", "share_name", "share"},
	    {"http://example.com/file", "http", "file", "endpoint", "http://example.com"},
	    {"https://example.com/file", "http", "file", "endpoint", "https://example.com"},
	    {"aliyun-drive://resource/file", "aliyun-drive", "file", "drive_type", "resource"},
	    {"b2://bucket/file", "b2", "file", "bucket", "bucket"},
	    {"cos://bucket/file", "cos", "file", "bucket", "bucket"},
	    {"dbfs://workspace.example.com/file", "dbfs", "file", "endpoint", "https://workspace.example.com"},
	    {"dropbox://remote/file", "dropbox", "file", "", ""},
	    {"ftp://ftp.example.com/file", "ftp", "file", "endpoint", "ftp://ftp.example.com"},
	    {"gdrive://service/file", "gdrive", "file", "", ""},
	    {"github://apache/opendal/file", "github", "file", "repo", "opendal"},
	    {"hf://datasets/apache/opendal/file", "hf", "file", "repo_id", "apache/opendal"},
	    {"huggingface://models/apache/opendal/file", "huggingface", "file", "repo_id", "apache/opendal"},
	    {"ipfs://ipfs.io/file", "ipfs", "file", "endpoint", "http://ipfs.io"},
	    {"koofr://api.koofr.net/user/file", "koofr", "file", "email", "user"},
	    {"lakefs://api.example.com/repository/main/file", "lakefs", "file", "repository", "repository"},
	    {"obs://bucket/file", "obs", "file", "bucket", "bucket"},
	    {"onedrive://drive/root/file", "onedrive", "file", "", ""},
	    {"oss://bucket/file", "oss", "file", "bucket", "bucket"},
	    {"pcloud://api.pcloud.com/file", "pcloud", "file", "endpoint", "https://api.pcloud.com"},
	    {"seafile://files.example.com/repository/file", "seafile", "file", "repo_name", "repository"},
	    {"sftp://sftp.example.com/file", "sftp", "file", "endpoint", "sftp.example.com"},
	    {"swift://swift.example.com/container/file", "swift", "file", "container", "container"},
	    {"tos://bucket/file", "tos", "file", "bucket", "bucket"},
	    {"upyun://bucket/file", "upyun", "file", "bucket", "bucket"},
	    {"vercel-artifacts://cache/file", "vercel-artifacts", "cache/file", "", ""},
	    {"vercel-blob://project/file", "vercel-blob", "file", "", ""},
	    {"webdav://dav.example.com/file", "webdav", "file", "endpoint", "https://dav.example.com"},
	    {"yandex-disk://disk/file", "yandex-disk", "file", "", ""},
	};

	for (const auto &test : cases) {
		OpenDALPath path;
		INFO(test.uri);
		REQUIRE(OpenDALPath::TryParse(test.uri, path));
		REQUIRE(path.scheme == test.scheme);
		REQUIRE(path.path == test.path);
		if (*test.config_key) {
			REQUIRE(path.uri_config.at(test.config_key) == test.config_value);
		}
	}
}

TEST_CASE("OpenDAL filesystem writes, gets file size, and reads from the memory backend", "[opendalfs]") {
	OpenDALFileSystem fs;
	auto writer =
	    fs.OpenFile("memory://hello.txt", FileFlags::FILE_FLAGS_WRITE | FileFlags::FILE_FLAGS_FILE_CREATE_NEW);
	REQUIRE(!fs.OnDiskFile(*writer));
	string contents = "hello world";
	REQUIRE(writer->Write(contents.data(), contents.size()) == contents.size());
	REQUIRE(writer->SeekPosition() == 11);
	REQUIRE(writer->GetFileSize() == 11);
	REQUIRE_THROWS(writer->Write(contents.data(), contents.size()));
	REQUIRE_THROWS(fs.Truncate(*writer, 5));
	fs.FileSync(*writer);
	writer->Close();
	writer->Close();

	auto op = make_uniq<opendal::Operator>("memory");
	op->Write("hello.txt", "hello world");
	OpenDALReadHandle reader(fs, std::move(op), "memory://hello.txt", "hello.txt", FileFlags::FILE_FLAGS_READ);
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

TEST_CASE("OpenDAL filesystem supports concurrent positional reads", "[opendalfs]") {
	OpenDALFileSystem fs;
	const string contents = "abcdefghijklmnopqrstuvwxyz";
	auto op = make_uniq<opendal::Operator>("memory");
	op->Write("parallel.txt", contents);
	OpenDALReadHandle reader(fs, std::move(op), "memory://parallel.txt", "parallel.txt",
	                         FileFlags::FILE_FLAGS_READ | FileFlags::FILE_FLAGS_PARALLEL_ACCESS);

	vector<string> results(8, string(3, '\0'));
	vector<idx_t> read_sizes(8);
	vector<std::thread> workers;
	for (idx_t index = 0; index < results.size(); index++) {
		workers.emplace_back([&reader, &results, &read_sizes, index]() {
			read_sizes[index] = reader.Read(results[index].data(), results[index].size(), index * 3);
		});
	}
	for (auto &worker : workers) {
		worker.join();
	}

	for (idx_t index = 0; index < results.size(); index++) {
		REQUIRE(read_sizes[index] == results[index].size());
		REQUIRE(results[index] == contents.substr(index * 3, 3));
	}
	REQUIRE(reader.Tell() == 0);
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
