#include "../precompiled.hpp"
#include "promotion_daemon.hpp"
#include <poseidon/http/client.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

namespace EmperyGoldScramble {

namespace {
	class PromotionClient : public Poseidon::TcpClientBase, private Poseidon::Http::ClientReader, private Poseidon::Http::ClientWriter {
	public:
		struct Response {
			Poseidon::Http::ResponseHeaders responseHeaders;
			Poseidon::StreamBuffer entity;
		};

	public:
		static boost::shared_ptr<Response> sendAndWait(const Poseidon::IpPort &addr, bool useSsl,
			Poseidon::Http::RequestHeaders requestHeaders, Poseidon::StreamBuffer entity)
		{
			PROFILE_ME;

			auto response = boost::make_shared<Response>();
			{
				const auto promise = boost::make_shared<Poseidon::JobPromise>();
				const auto client = boost::shared_ptr<PromotionClient>(new PromotionClient(addr, useSsl, response, promise));
				                    // boost::make_shared<PromotionClient>(addr, useSsl, response, promise);
				client->goResident();
				client->Poseidon::Http::ClientWriter::putRequest(std::move(requestHeaders), std::move(entity));
				Poseidon::JobDispatcher::yield(promise);
				promise->checkAndRethrow();
			}
			return response;
		}

	private:
		const boost::shared_ptr<Response> m_response;
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;

	private:
		PromotionClient(const Poseidon::IpPort &addr, bool useSsl,
			boost::shared_ptr<Response> response, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::TcpClientBase(addr, useSsl)
			, m_response(std::move(response)), m_promise(std::move(promise))
		{
		}

	public:
		~PromotionClient(){
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
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Connection to promotion server closed: errCode = ", errCode);

			if(!m_promise->isSatisfied()){
				try {
					m_promise->setException(boost::copy_exception(std::runtime_error("Lost connection to cluster server")));
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

			m_response->responseHeaders = std::move(responseHeaders);
		}
		void onResponseEntity(boost::uint64_t entityOffset, bool /* isChunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Entity received: entityOffset = ", entityOffset, ", entitySize = ", entity.size());

			m_response->entity.splice(entity);
		}
		bool onResponseEnd(boost::uint64_t contentLength, bool /* isChunked */, Poseidon::OptionalMap headers) override {
			PROFILE_ME;
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Entity terminated: contentLength = ", contentLength);

			for(auto it = headers.begin(); it != headers.end(); ++it){
				m_response->responseHeaders.headers.append(it->first, std::move(it->second));
			}

			m_promise->setSuccess();

			return true;
		}

		long onEncodedDataAvail(Poseidon::StreamBuffer encoded) override {
			PROFILE_ME;

			return Poseidon::TcpClientBase::send(std::move(encoded));
		}
	};
}

std::string PromotionDaemon::getLoginNameFromSessionId(const std::string &sessionId){
	PROFILE_ME;

	return {};
}

}
