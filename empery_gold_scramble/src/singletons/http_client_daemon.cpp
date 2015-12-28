#include "../precompiled.hpp"
#include "http_client_daemon.hpp"
#include <poseidon/http/client.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/atomic.hpp>
#include <poseidon/http/utilities.hpp>
#include <netdb.h>
#include <unistd.h>

namespace EmperyGoldScramble {

namespace {
	volatile std::size_t g_pending_callback_count = 0;

	struct DnsCallbackParam : NONCOPYABLE {
		Poseidon::SockAddr &sock_addr;

		std::string host;
		unsigned port;
		boost::shared_ptr<Poseidon::JobPromise> promise;

		std::string host_str;
		char port_str[16];
		::gaicb cb;
		::gaicb *req;

		DnsCallbackParam(Poseidon::SockAddr &sock_addr_,
			std::string host_, unsigned port_, boost::shared_ptr<Poseidon::JobPromise> promise_)
			: sock_addr(sock_addr_)
			, host(std::move(host_)), port(port_), promise(std::move(promise_))
		{
			assert(!host.empty());

			if((host.begin()[0] == '[') && (host.end()[-1] == ']')){
				host_str.assign(host.begin() + 1, host.end() - 1);
			} else {
				host_str = host;
			}
			std::sprintf(port_str, "%u", port);

			cb.ar_name = host_str.c_str();
			cb.ar_service = port_str;
			cb.ar_request = nullptr;
			cb.ar_result = nullptr;
			req = &cb;
		}
		~DnsCallbackParam(){
			if(cb.ar_result){
				::freeaddrinfo(cb.ar_result);
			}
		}
	};

	void dns_callback(::sigval sigval_param) noexcept {
		PROFILE_ME;

		Poseidon::Logger::set_thread_tag("   D"); // DNS
		const boost::scoped_ptr<DnsCallbackParam> param(static_cast<DnsCallbackParam *>(sigval_param.sival_ptr));

		try {
			const int gai_code = ::gai_error(param->req);
			const char *err_msg = "";
			if(gai_code != 0){
				err_msg = ::gai_strerror(gai_code);
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("DNS lookup failure: host = ", param->host, ", gai_code = ", gai_code, ", err_msg = ", err_msg);
				DEBUG_THROW(Exception, SharedNts(err_msg));
			}
			param->sock_addr = Poseidon::SockAddr(param->cb.ar_result->ai_addr, param->cb.ar_result->ai_addrlen);
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("DNS lookup success: host:port = ", param->host, ':', param->port,
				", result = ", Poseidon::get_ip_port_from_sock_addr(param->sock_addr));
			param->promise->set_success();
		} catch(std::exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE_INFO("std::exception thrown in DNS loop: what = ", e.what());
			// param->promise->set_exception(boost::current_exception());
			param->promise->set_exception(boost::copy_exception(e));
		}

		Poseidon::atomic_sub(g_pending_callback_count, 1, Poseidon::ATOMIC_RELAXED);
	}

	struct CallbackCancellationGuard {
		~CallbackCancellationGuard(){
			for(;;){
				const auto count = Poseidon::atomic_load(g_pending_callback_count, Poseidon::ATOMIC_RELAXED);
				if(count == 0){
					break;
				}
				LOG_EMPERY_GOLD_SCRAMBLE_INFO("Waiting for ", count, " pending DNS callbacks...");
				::gai_cancel(nullptr);
				::usleep(100000);
			}
		}
	};

	MODULE_RAII(handles){
		handles.push(boost::make_shared<CallbackCancellationGuard>());
	}

	class HttpClient : public Poseidon::TcpClientBase, private Poseidon::Http::ClientReader, private Poseidon::Http::ClientWriter {
	public:
		static HttpClientDaemon::Response send_and_wait(const std::string &host, unsigned port, bool use_ssl,
			Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity)
		{
			PROFILE_ME;

			Poseidon::SockAddr sock_addr;
			{
				const auto promise = boost::make_shared<Poseidon::JobPromise>();
				const auto param = new DnsCallbackParam(sock_addr, host, port, promise);
				try {
					::sigevent sev;
					sev.sigev_notify = SIGEV_THREAD;
					sev.sigev_value.sival_ptr = param;
					sev.sigev_notify_function = &dns_callback;
					sev.sigev_notify_attributes = nullptr;
					const int gai_code = ::getaddrinfo_a(GAI_NOWAIT, &(param->req), 1, &sev); // noexcept
					if(gai_code != 0){
						LOG_EMPERY_GOLD_SCRAMBLE_WARNING("DNS failure: gai_code = ", gai_code, ", err_msg = ", ::gai_strerror(gai_code));
						DEBUG_THROW(Exception, sslit("DNS failure"));
					}
					Poseidon::atomic_add(g_pending_callback_count, 1, Poseidon::ATOMIC_RELAXED); // noexcept
				} catch(...){
					delete param;
					throw;
				}
				Poseidon::JobDispatcher::yield(promise);
				promise->check_and_rethrow();
			}

			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			const auto client = boost::shared_ptr<HttpClient>(new HttpClient(sock_addr, use_ssl, promise));
			                    // boost::make_shared<HttpClient>(sock_addr, use_ssl, promise);
			client->go_resident();
			client->Poseidon::Http::ClientWriter::put_request(std::move(request_headers), std::move(entity));
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
			return std::move(client->m_response);
		}

	private:
		boost::shared_ptr<Poseidon::JobPromise> m_promise;
		HttpClientDaemon::Response m_response;

	private:
		HttpClient(const Poseidon::SockAddr &sock_addr, bool use_ssl, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::TcpClientBase(sock_addr, use_ssl)
			, m_promise(std::move(promise)), m_response()
		{
		}

	protected:
		void on_read_hup() noexcept override {
			PROFILE_ME;

			try {
				if(Poseidon::Http::ClientReader::is_content_till_eof()){
					terminate_content();
				}
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
				force_shutdown();
			}

			TcpClientBase::on_read_hup();
		}
		void on_close(int err_code) noexcept override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Connection to HTTP server closed: err_code = ", err_code);

			if(!m_promise->is_satisfied()){
				try {
					DEBUG_THROW(Exception, sslit("Lost connection to HTTP server"));
				} catch(Exception &e){
					m_promise->set_exception(boost::copy_exception(e));
				} catch(std::exception &e){
					LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
					m_promise->set_exception(boost::copy_exception(e));
				}
			}

			TcpClientBase::on_close(err_code);
		}

		void on_read_avail(Poseidon::StreamBuffer data) override {
			PROFILE_ME;

			Poseidon::Http::ClientReader::put_encoded_data(std::move(data));
		}

		void on_response_headers(Poseidon::Http::ResponseHeaders response_headers,
			std::string /* transfer_encoding */, std::uint64_t /* content_length */) override
		{
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Response headers received: status_code = ", response_headers.status_code);

			m_response.status_code = response_headers.status_code;
			m_response.content_type = response_headers.headers.get("Content-Type");
		}
		void on_response_entity(std::uint64_t entity_offset, bool /* is_chunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Entity received: entity_offset = ", entity_offset, ", entity_size = ", entity.size());

			m_response.entity.splice(entity);
		}
		bool on_response_end(std::uint64_t content_length, bool /* is_chunked */, Poseidon::OptionalMap /* headers */) override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Entity terminated: content_length = ", content_length);

			m_promise->set_success();

			return true;
		}

		long on_encoded_data_avail(Poseidon::StreamBuffer encoded) override {
			PROFILE_ME;

			return Poseidon::TcpClientBase::send(std::move(encoded));
		}
	};
}

HttpClientDaemon::Response HttpClientDaemon::get(const std::string &host, unsigned port, bool use_ssl, const std::string &auth,
	std::string path, Poseidon::OptionalMap params)
{
	Poseidon::Http::RequestHeaders request_headers;
	request_headers.verb = Poseidon::Http::V_GET;
	request_headers.uri = std::move(path);
	request_headers.version = 10001;
	request_headers.get_params = std::move(params);
	request_headers.headers.set(sslit("Host"), host);
	request_headers.headers.set(sslit("Connection"), "Close");
	if(!auth.empty()){
		request_headers.headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(auth));
	}
	return HttpClient::send_and_wait(host, port, use_ssl, std::move(request_headers), { });
}
HttpClientDaemon::Response HttpClientDaemon::post(const std::string &host, unsigned port, bool use_ssl, const std::string &auth,
	std::string path, Poseidon::OptionalMap params, Poseidon::StreamBuffer entity, std::string mime_type)
{
	Poseidon::Http::RequestHeaders request_headers;
	request_headers.verb = Poseidon::Http::V_POST;
	request_headers.uri = std::move(path);
	request_headers.version = 10001;
	request_headers.get_params = std::move(params);
	request_headers.headers.set(sslit("Host"), host);
	request_headers.headers.set(sslit("Connection"), "Close");
	if(!auth.empty()){
		request_headers.headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(auth));
	}
	request_headers.headers.set(sslit("Content-Type"), std::move(mime_type));
	return HttpClient::send_and_wait(host, port, use_ssl, std::move(request_headers), std::move(entity));
}

}
