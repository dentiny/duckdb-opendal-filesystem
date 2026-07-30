#include "catch/catch.hpp"
#include "opendal_file_system.hpp"
#include "opendal_layer_options.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace duckdb {

namespace {

class MockHttpService {
public:
	enum class Mode { DELAY, RETRY_THEN_SUCCEED, SUCCEED };

	MockHttpService(Mode mode_p, idx_t expected_requests_p, std::chrono::milliseconds delay_p = {})
	    : mode(mode_p), expected_requests(expected_requests_p), delay(delay_p) {
		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd < 0) {
			throw std::runtime_error("could not create mock HTTP socket");
		}
		int reuse = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
		sockaddr_in address {};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		address.sin_port = 0;
		if (bind(server_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
		    listen(server_fd, 8) != 0) {
			close(server_fd);
			throw std::runtime_error("could not bind mock HTTP socket");
		}
		socklen_t address_size = sizeof(address);
		if (getsockname(server_fd, reinterpret_cast<sockaddr *>(&address), &address_size) != 0) {
			close(server_fd);
			throw std::runtime_error("could not inspect mock HTTP socket");
		}
		port = ntohs(address.sin_port);
		worker = std::thread([this]() { Serve(); });
	}

	~MockHttpService() {
		if (server_fd >= 0) {
			shutdown(server_fd, SHUT_RDWR);
			close(server_fd);
			server_fd = -1;
		}
		if (worker.joinable()) {
			worker.join();
		}
	}

	string Endpoint() const {
		return "http://127.0.0.1:" + std::to_string(port);
	}

	idx_t RequestCount() const {
		return request_count.load();
	}

private:
	void Serve() {
		while (request_count.load() < expected_requests) {
			auto client_fd = accept(server_fd, nullptr, nullptr);
			if (client_fd < 0) {
				return;
			}
			char request[4096];
			recv(client_fd, request, sizeof(request), 0);
			const auto current = ++request_count;
			if (mode == Mode::DELAY) {
				std::this_thread::sleep_for(delay);
			} else {
				const char *response =
				    mode == Mode::RETRY_THEN_SUCCEED && current < expected_requests
				        ? "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
				        : "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
				send(client_fd, response, std::strlen(response), 0);
			}
			shutdown(client_fd, SHUT_RDWR);
			close(client_fd);
		}
	}

private:
	Mode mode;
	idx_t expected_requests;
	std::chrono::milliseconds delay;
	std::atomic<idx_t> request_count {0};
	int server_fd = -1;
	uint16_t port = 0;
	std::thread worker;
};

std::vector<std::unique_ptr<opendal::OperatorOption>>
ToOpenDALOperatorOptions(vector<unique_ptr<opendal::OperatorOption>> options_p) {
	std::vector<std::unique_ptr<opendal::OperatorOption>> result;
	result.reserve(options_p.size());
	for (auto &option : options_p) {
		result.push_back(std::unique_ptr<opendal::OperatorOption>(option.release()));
	}
	return result;
}

} // namespace

TEST_CASE("OpenDAL timeout layer stops a stalled service request", "[opendalfs][layers]") {
	MockHttpService service(MockHttpService::Mode::DELAY, 1, std::chrono::milliseconds(500));
	std::vector<std::unique_ptr<opendal::OperatorOption>> layers;
	layers.push_back(opendal::WithTimeout(std::chrono::milliseconds(25), std::chrono::milliseconds(25)));
	opendal::Operator op("http", {{"endpoint", service.Endpoint()}}, std::move(layers));
	const auto started = std::chrono::steady_clock::now();
	REQUIRE_THROWS(op.Stat("stalled.csv"));
	const auto elapsed = std::chrono::steady_clock::now() - started;
	REQUIRE(service.RequestCount() == 1);
	REQUIRE(elapsed < std::chrono::milliseconds(300));
}

TEST_CASE("OpenDAL retry layer retries temporary service failures", "[opendalfs][layers]") {
	MockHttpService service(MockHttpService::Mode::RETRY_THEN_SUCCEED, 3);
	OpenDALLayerOptions layers;
	layers.timeout_ms = 1000;
	layers.io_timeout_ms = 1000;
	layers.retry_max_times = 3;
	layers.retry_min_delay_ms = 1;
	layers.retry_max_delay_ms = 1;
	layers.retry_jitter = false;

	opendal::Operator op("http", {{"endpoint", service.Endpoint()}},
	                     ToOpenDALOperatorOptions(layers.ToOperatorOptions()));
	REQUIRE_NOTHROW(op.Stat("eventually-available.csv"));
	REQUIRE(service.RequestCount() == 3);
}

TEST_CASE("OpenDAL read handles cache metadata after the first size lookup", "[opendalfs][layers]") {
	MockHttpService service(MockHttpService::Mode::SUCCEED, 4);
	OpenDALFileSystem fs;
	auto handle = fs.OpenFile(service.Endpoint() + "/cached.csv", FileFlags::FILE_FLAGS_READ);

	REQUIRE(handle->GetFileSize() == 0);
	const auto requests_after_first_size = service.RequestCount();
	REQUIRE(handle->GetFileSize() == 0);
	REQUIRE(service.RequestCount() == requests_after_first_size);
}

TEST_CASE("OpenDAL layer options are only pushed when enabled", "[opendalfs][layers]") {
	OpenDALLayerOptions layers;
	REQUIRE(layers.ToOperatorOptions().size() == 2);

	layers.timeout_layer_enabled = false;
	REQUIRE(layers.ToOperatorOptions().size() == 1);

	layers.retry_layer_enabled = false;
	REQUIRE(layers.ToOperatorOptions().empty());
}
} // namespace duckdb
