#include "../precompiled.hpp"
#include "simple_http_client_daemon.hpp"
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/dns_daemon.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/http/client.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/system_exception.hpp>

namespace EmperyCenter {

namespace {
	class SimpleHttpClient : public Poseidon::Http::Client {
	private:
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;

		Poseidon::Http::ResponseHeaders m_response_headers;
		std::string m_transfer_encoding;
		Poseidon::StreamBuffer m_entity;

	public:
		SimpleHttpClient(const Poseidon::SockAddr &addr, bool use_ssl, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::Http::Client(addr, use_ssl)
			, m_promise(std::move(promise))
		{
		}

	protected:
		void on_sync_response(Poseidon::Http::ResponseHeaders response_headers,
			std::string transfer_encoding, Poseidon::StreamBuffer entity) override
		{
			PROFILE_ME;

			m_response_headers = std::move(response_headers);
			m_transfer_encoding = std::move(transfer_encoding);
			m_entity = std::move(entity);
		}

	public:
		Poseidon::Http::ResponseHeaders &get_response_headers(){
			return m_response_headers;
		}
		std::string &get_transfer_encoding(){
			return m_transfer_encoding;
		}
		Poseidon::StreamBuffer &get_entity(){
			return m_entity;
		}
	};
}

Poseidon::StreamBuffer SimpleHttpClientDaemon::sync_request(
	const std::string &host, unsigned port, bool use_ssl, Poseidon::Http::Verb verb, std::string uri,
	Poseidon::OptionalMap get_params, const std::string &user_pass, Poseidon::StreamBuffer entity, std::string content_type)
{
	PROFILE_ME;

	const auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
	{
		const auto promise = Poseidon::DnsDaemon::enqueue_for_looking_up(sock_addr, host, port);
		Poseidon::JobDispatcher::yield(promise, true);
	}

	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	const auto client = boost::make_shared<SimpleHttpClient>(*sock_addr, use_ssl, promise);
	client->go_resident();

	Poseidon::Http::RequestHeaders req_headers;
	req_headers.verb       = verb;
	req_headers.uri        = std::move(uri);
	req_headers.version    = 10001;
	req_headers.get_params = std::move(get_params);
	req_headers.headers.set(sslit("Host"),       host);
	req_headers.headers.set(sslit("Connection"), "Close");
	if(!user_pass.empty()){
		req_headers.headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(user_pass));
	}
	if(!content_type.empty()){
		req_headers.headers.set(sslit("Content-Type"), std::move(content_type));
	}
	if(!client->send(std::move(req_headers), std::move(entity))){
		LOG_EMPERY_CENTER_WARNING("Failed to send data to remote server!");
		DEBUG_THROW(Exception, sslit("Failed to send data to remote server"));
	}
	Poseidon::JobDispatcher::yield(promise, true);

	return std::move(client->get_entity());
}

}
