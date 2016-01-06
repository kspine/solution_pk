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
#include <poseidon/async_job.hpp>
#include <poseidon/system_exception.hpp>

namespace EmperyCenter {

namespace {
	class SimpleHttpClient : public Poseidon::Http::Client {
	private:
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;
		const boost::shared_ptr<Poseidon::StreamBuffer> m_buffer;

		Poseidon::Http::ResponseHeaders m_res_headers;

	public:
		SimpleHttpClient(const Poseidon::SockAddr &addr, bool use_ssl,
			boost::shared_ptr<Poseidon::JobPromise> promise, boost::shared_ptr<Poseidon::StreamBuffer> buffer)
			: Poseidon::Http::Client(addr, use_ssl)
			, m_promise(std::move(promise)), m_buffer(std::move(buffer))
			, m_res_headers()
		{
		}

	protected:
		void on_close(int err_code) noexcept override {
			PROFILE_ME;

			try {
				Poseidon::enqueue_async_job(
					virtual_weak_from_this<SimpleHttpClient>(),
					std::bind(
						[](const boost::shared_ptr<Poseidon::JobPromise> &promise, int err_code){
							if(!promise->is_satisfied()){
								promise->set_exception(boost::copy_exception(Poseidon::SystemException(__FILE__, __LINE__, err_code)));
							}
						},
						m_promise, err_code)
				);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}

			Poseidon::Http::Client::on_close(err_code);
		}

		void on_sync_response_headers(Poseidon::Http::ResponseHeaders response_headers,
			std::string /* transfer_encoding */, std::uint64_t /* content_length */) override
		{
			PROFILE_ME;

			m_res_headers = std::move(response_headers);
		}
		void on_sync_response_entity(std::uint64_t /* entity_offset */, bool /* is_chunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;

			m_buffer->splice(entity);
		}
		void on_sync_response_end(std::uint64_t /* content_length */, bool /* is_chunked */, Poseidon::OptionalMap /* headers */) override {
			PROFILE_ME;

			LOG_EMPERY_CENTER_DEBUG("Received response: status_code = ", m_res_headers.status_code, ", entity = ", m_buffer->dump());
			if(m_res_headers.status_code / 100 == 2){
				m_promise->set_success();
			} else {
				LOG_EMPERY_CENTER_ERROR("Received HTTP error from remote server: status_code = ", m_res_headers.status_code);
				m_promise->set_exception(boost::copy_exception(
					Poseidon::Http::Exception(__FILE__, __LINE__, m_res_headers.status_code, { }, SharedNts(m_buffer->dump()))
					));
			}
		}
	};
}

Poseidon::StreamBuffer SimpleHttpClientDaemon::sync_request(
	const std::string &host, unsigned port, bool use_ssl, Poseidon::Http::Verb verb, std::string uri,
	Poseidon::OptionalMap get_params, const std::string &user_pass, Poseidon::StreamBuffer entity, std::string content_type)
{
	PROFILE_ME;

	const auto buffer = boost::make_shared<Poseidon::StreamBuffer>();
	const auto promise = async_request(std::move(buffer),
		host, port, use_ssl, verb, std::move(uri), std::move(get_params), std::move(user_pass), std::move(entity), std::move(content_type));
	Poseidon::JobDispatcher::yield(promise);
	promise->check_and_rethrow();
	return std::move(*buffer);
}

boost::shared_ptr<const Poseidon::JobPromise> SimpleHttpClientDaemon::async_request(boost::shared_ptr<Poseidon::StreamBuffer> ret,
	const std::string &host, unsigned port, bool use_ssl, Poseidon::Http::Verb verb, std::string uri,
	Poseidon::OptionalMap get_params, const std::string &user_pass, Poseidon::StreamBuffer entity, std::string content_type)
{
	PROFILE_ME;

	const auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
	{
		const auto promise = Poseidon::DnsDaemon::enqueue_for_looking_up(sock_addr, host, port);
		Poseidon::JobDispatcher::yield(promise);
		promise->check_and_rethrow();
	}

	auto promise = boost::make_shared<Poseidon::JobPromise>();
	const auto client = boost::make_shared<SimpleHttpClient>(*sock_addr, use_ssl, promise, std::move(ret));
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

	return std::move(promise);
}

}
