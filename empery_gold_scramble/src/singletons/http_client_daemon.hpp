#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_HTTP_CLIENT_DAEMON_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_HTTP_CLIENT_DAEMON_HPP_

#include <string>
#include <poseidon/fwd.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/optional_map.hpp>

namespace EmperyGoldScramble {

struct HttpClientDaemon {
	struct Response {
		Poseidon::Http::StatusCode status_code;
		std::string content_type;
		Poseidon::StreamBuffer entity;
	};

	static Response get(const std::string &host, unsigned port, bool use_ssl, const std::string &auth,
		std::string path, Poseidon::OptionalMap params);
	static Response post(const std::string &host, unsigned port, bool use_ssl, const std::string &auth,
		std::string path, Poseidon::OptionalMap params, Poseidon::StreamBuffer entity, std::string mime_type);

private:
	HttpClientDaemon() = delete;
};

}

#endif
