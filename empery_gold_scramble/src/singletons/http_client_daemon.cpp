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
	volatile std::size_t g_pendingCallbackCount = 0;

	struct DnsCallbackParam : NONCOPYABLE {
		Poseidon::SockAddr &sockAddr;

		std::string host;
		unsigned port;
		boost::shared_ptr<Poseidon::JobPromise> promise;

		std::string hostStr;
		char portStr[16];
		::gaicb cb;
		::gaicb *req;

		DnsCallbackParam(Poseidon::SockAddr &sockAddr_,
			std::string host_, unsigned port_, boost::shared_ptr<Poseidon::JobPromise> promise_)
			: sockAddr(sockAddr_)
			, host(std::move(host_)), port(port_), promise(std::move(promise_))
		{
			assert(!host.empty());

			if((host.begin()[0] == '[') && (host.end()[-1] == ']')){
				hostStr.assign(host.begin() + 1, host.end() - 1);
			} else {
				hostStr = host;
			}
			std::sprintf(portStr, "%u", port);

			cb.ar_name = hostStr.c_str();
			cb.ar_service = portStr;
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

	void dnsCallback(::sigval sigvalParam) noexcept {
		PROFILE_ME;

		Poseidon::Logger::setThreadTag("   D"); // DNS
		const boost::scoped_ptr<DnsCallbackParam> param(static_cast<DnsCallbackParam *>(sigvalParam.sival_ptr));

		try {
			const int gaiCode = ::gai_error(param->req);
			const char *errMsg = "";
			if(gaiCode != 0){
				errMsg = ::gai_strerror(gaiCode);
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("DNS lookup failure: host = ", param->host, ", gaiCode = ", gaiCode, ", errMsg = ", errMsg);
				DEBUG_THROW(Exception, SharedNts(errMsg));
			}
			param->sockAddr = Poseidon::SockAddr(param->cb.ar_result->ai_addr, param->cb.ar_result->ai_addrlen);
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("DNS lookup success: host:port = ", param->host, ':', param->port,
				", result = ", Poseidon::getIpPortFromSockAddr(param->sockAddr));
			param->promise->setSuccess();
		} catch(std::exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE_INFO("std::exception thrown in DNS loop: what = ", e.what());
			// param->promise->setException(boost::current_exception());
			param->promise->setException(boost::copy_exception(e));
		}

		Poseidon::atomicSub(g_pendingCallbackCount, 1, Poseidon::ATOMIC_RELAXED);
	}

	struct CallbackCancellationGuard {
		~CallbackCancellationGuard(){
			for(;;){
				const auto count = Poseidon::atomicLoad(g_pendingCallbackCount, Poseidon::ATOMIC_RELAXED);
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
		static HttpClientDaemon::Response sendAndWait(const std::string &host, unsigned port, bool useSsl,
			Poseidon::Http::RequestHeaders requestHeaders, Poseidon::StreamBuffer entity)
		{
			PROFILE_ME;

			Poseidon::SockAddr sockAddr;
			{
				const auto promise = boost::make_shared<Poseidon::JobPromise>();
				const auto param = new DnsCallbackParam(sockAddr, host, port, promise);
				try {
					::sigevent sev;
					sev.sigev_notify = SIGEV_THREAD;
					sev.sigev_value.sival_ptr = param;
					sev.sigev_notify_function = &dnsCallback;
					sev.sigev_notify_attributes = nullptr;
					const int gaiCode = ::getaddrinfo_a(GAI_NOWAIT, &(param->req), 1, &sev); // noexcept
					if(gaiCode != 0){
						LOG_EMPERY_GOLD_SCRAMBLE_WARNING("DNS failure: gaiCode = ", gaiCode, ", errMsg = ", ::gai_strerror(gaiCode));
						DEBUG_THROW(Exception, sslit("DNS failure"));
					}
					Poseidon::atomicAdd(g_pendingCallbackCount, 1, Poseidon::ATOMIC_RELAXED); // noexcept
				} catch(...){
					delete param;
					throw;
				}
				Poseidon::JobDispatcher::yield(promise);
				promise->checkAndRethrow();
			}

			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			const auto client = boost::shared_ptr<HttpClient>(new HttpClient(sockAddr, useSsl, promise));
			                    // boost::make_shared<HttpClient>(sockAddr, useSsl, promise);
			client->goResident();
			client->Poseidon::Http::ClientWriter::putRequest(std::move(requestHeaders), std::move(entity));
			Poseidon::JobDispatcher::yield(promise);
			promise->checkAndRethrow();
			return std::move(client->m_response);
		}

	private:
		boost::shared_ptr<Poseidon::JobPromise> m_promise;
		HttpClientDaemon::Response m_response;

	private:
		HttpClient(const Poseidon::SockAddr &sockAddr, bool useSsl, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::TcpClientBase(sockAddr, useSsl)
			, m_promise(std::move(promise)), m_response()
		{
		}

	protected:
		void onReadHup() noexcept override {
			PROFILE_ME;

			try {
				if(Poseidon::Http::ClientReader::isContentTillEof()){
					terminateContent();
				}
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
				forceShutdown();
			}

			TcpClientBase::onReadHup();
		}
		void onClose(int errCode) noexcept override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Connection to HTTP server closed: errCode = ", errCode);

			if(!m_promise->isSatisfied()){
				try {
					DEBUG_THROW(Exception, sslit("Lost connection to HTTP server"));
				} catch(Exception &e){
					m_promise->setException(boost::copy_exception(e));
				} catch(std::exception &e){
					LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
					m_promise->setException(boost::copy_exception(e));
				}
			}

			TcpClientBase::onClose(errCode);
		}

		void onReadAvail(Poseidon::StreamBuffer data) override {
			PROFILE_ME;

			Poseidon::Http::ClientReader::putEncodedData(std::move(data));
		}

		void onResponseHeaders(Poseidon::Http::ResponseHeaders responseHeaders,
			std::string /* transferEncoding */, boost::uint64_t /* contentLength */) override
		{
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Response headers received: statusCode = ", responseHeaders.statusCode);

			m_response.statusCode = responseHeaders.statusCode;
			m_response.contentType = responseHeaders.headers.get("Content-Type");
		}
		void onResponseEntity(boost::uint64_t entityOffset, bool /* isChunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Entity received: entityOffset = ", entityOffset, ", entitySize = ", entity.size());

			m_response.entity.splice(entity);
		}
		bool onResponseEnd(boost::uint64_t contentLength, bool /* isChunked */, Poseidon::OptionalMap /* headers */) override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Entity terminated: contentLength = ", contentLength);

			m_promise->setSuccess();

			return true;
		}

		long onEncodedDataAvail(Poseidon::StreamBuffer encoded) override {
			PROFILE_ME;

			return Poseidon::TcpClientBase::send(std::move(encoded));
		}
	};
}

HttpClientDaemon::Response HttpClientDaemon::get(const std::string &host, unsigned port, bool useSsl, const std::string &auth,
	std::string path, Poseidon::OptionalMap params)
{
	Poseidon::Http::RequestHeaders requestHeaders;
	requestHeaders.verb = Poseidon::Http::V_GET;
	requestHeaders.uri = std::move(path);
	requestHeaders.version = 10001;
	requestHeaders.getParams = std::move(params);
	requestHeaders.headers.set(sslit("Host"), host);
	requestHeaders.headers.set(sslit("Connection"), "Close");
	if(!auth.empty()){
		requestHeaders.headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64Encode(auth));
	}
	return HttpClient::sendAndWait(host, port, useSsl, std::move(requestHeaders), { });
}
HttpClientDaemon::Response HttpClientDaemon::post(const std::string &host, unsigned port, bool useSsl, const std::string &auth,
	std::string path, Poseidon::OptionalMap params, Poseidon::StreamBuffer entity, std::string mimeType)
{
	Poseidon::Http::RequestHeaders requestHeaders;
	requestHeaders.verb = Poseidon::Http::V_POST;
	requestHeaders.uri = std::move(path);
	requestHeaders.version = 10001;
	requestHeaders.getParams = std::move(params);
	requestHeaders.headers.set(sslit("Host"), host);
	requestHeaders.headers.set(sslit("Connection"), "Close");
	if(!auth.empty()){
		requestHeaders.headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64Encode(auth));
	}
	requestHeaders.headers.set(sslit("Content-Type"), std::move(mimeType));
	return HttpClient::sendAndWait(host, port, useSsl, std::move(requestHeaders), std::move(entity));
}

}
