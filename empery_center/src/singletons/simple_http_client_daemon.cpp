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
	class SimpleHttpClient : public Poseidon::TcpClientBase, private Poseidon::Http::ClientReader, private Poseidon::Http::ClientWriter {
	private:
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;

		Poseidon::Http::ResponseHeaders m_res_headers;
		Poseidon::StreamBuffer m_buffer;

	public:
		SimpleHttpClient(const Poseidon::SockAddr &addr, bool use_ssl, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::TcpClientBase(addr, use_ssl)
			, m_promise(std::move(promise))
			, m_res_headers(), m_buffer()
		{
		}

	protected:
		void on_read_hup() noexcept {
			PROFILE_ME;

			try {
				if(Poseidon::Http::ClientReader::is_content_till_eof()){
					Poseidon::Http::ClientReader::terminate_content();
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				force_shutdown();
			} catch(...){
				LOG_EMPERY_CENTER_WARNING("Unknown exception thrown");
				force_shutdown();
			}

			Poseidon::TcpClientBase::on_read_hup();
		}
		void on_close(int err_code) noexcept override {
			PROFILE_ME;

			if(!m_promise->is_satisfied()){
				try {
					try {
						DEBUG_THROW(Poseidon::SystemException, err_code);
					} catch(Poseidon::SystemException &e){
						m_promise->set_exception(boost::copy_exception(e));
					}
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
					m_promise->set_exception(boost::copy_exception(std::bad_exception()));
				}
			}

			Poseidon::TcpClientBase::on_close(err_code);
		}

		// Poseidon::Http::ClientReader
		void on_read_avail(Poseidon::StreamBuffer data){
			PROFILE_ME;

			Poseidon::Http::ClientReader::put_encoded_data(std::move(data));
		}

		void on_response_headers(Poseidon::Http::ResponseHeaders response_headers,
			std::string transfer_encoding, std::uint64_t content_length) override
		{
			PROFILE_ME;
			LOG_EMPERY_CENTER_DEBUG("Response headers: status_code = ", response_headers.status_code, ", reason = ", response_headers.reason,
				", transfer_encoding = ", transfer_encoding, ", content_length = ", content_length);

			m_res_headers = std::move(response_headers);
		}
		void on_response_entity(std::uint64_t entity_offset, bool /* is_chunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;
			LOG_EMPERY_CENTER_DEBUG("Response entity: entity_offset = ", entity_offset, ", entity.size() = ", entity.size());

			m_buffer.splice(entity);
		}
		bool on_response_end(std::uint64_t content_length, bool /* is_chunked */, Poseidon::OptionalMap /* headers */) override {
			PROFILE_ME;
			LOG_EMPERY_CENTER_DEBUG("Response end: content_length = ", content_length);

			if(m_res_headers.status_code / 100 == 2){
				m_promise->set_success();
			} else {
				try {
					DEBUG_THROW(Poseidon::Http::Exception, m_res_headers.status_code, { }, SharedNts(m_buffer.dump()));
				} catch(Poseidon::Http::Exception &e){
					m_promise->set_exception(boost::copy_exception(e));
				}
			}

			return true;
		}

		// Poseidon::Http::ClientWriter
		long on_encoded_data_avail(Poseidon::StreamBuffer encoded) override {
			PROFILE_ME;

			return Poseidon::TcpClientBase::send(std::move(encoded));
		}

	public:
		const Poseidon::StreamBuffer &get_buffer() const {
			return m_buffer;
		}
		Poseidon::StreamBuffer &get_buffer(){
			return m_buffer;
		}

		bool send(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity){
			return Poseidon::Http::ClientWriter::put_request(std::move(request_headers), std::move(entity));
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

	return std::move(client->get_buffer());
}

}
