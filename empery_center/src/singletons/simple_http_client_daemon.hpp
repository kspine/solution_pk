#ifndef EMPERY_CENTER_SINGLETONS_SIMPLE_HTTP_CLIENT_DAEMON_HPP_
#define EMPERY_CENTER_SINGLETONS_SIMPLE_HTTP_CLIENT_DAEMON_HPP_

#include <poseidon/fwd.hpp>
#include <poseidon/http/verbs.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/optional_map.hpp>
#include <boost/shared_ptr.hpp>

namespace EmperyCenter {

struct SimpleHttpClientDaemon {
	// 同步接口。
	static Poseidon::StreamBuffer sync_request(
		const std::string &host, unsigned port, bool use_ssl, Poseidon::Http::Verb verb, std::string uri,
		Poseidon::OptionalMap get_params = Poseidon::OptionalMap(), const std::string &user_pass = std::string(),
		Poseidon::StreamBuffer entity = Poseidon::StreamBuffer(), std::string content_type = std::string());

private:
	SimpleHttpClientDaemon() = delete;
};

}

#endif
