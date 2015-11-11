#include "precompiled.hpp"
#include "cluster_client.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/async_job.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include "../../empery_center/src/msg/g_packed.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"

namespace EmperyCluster {

namespace Msg {
	using namespace EmperyCenter::Msg;
}

using Result          = ClusterClient::Result;
using ServletCallback = ClusterClient::ServletCallback;

namespace {
	std::map<unsigned, boost::weak_ptr<const ServletCallback> > g_servletMap;
}

boost::shared_ptr<const ServletCallback> ClusterClient::createServlet(boost::uint16_t messageId, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	const auto result = g_servletMap.insert(std::make_pair(messageId, servlet));
	if(!result.second){
		if(!result.first->second.expired()){
			LOG_EMPERY_CLUSTER_ERROR("Duplicate cluster servlet: messageId = ", messageId);
			DEBUG_THROW(Exception, sslit("Duplicate cluster servlet"));
		}
		result.first->second = servlet;
	}
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> ClusterClient::getServlet(boost::uint16_t messageId){
	PROFILE_ME;

	const auto it = g_servletMap.find(messageId);
	if(it == g_servletMap.end()){
		return { };
	}
	auto servlet = it->second.lock();
	if(!servlet){
		g_servletMap.erase(it);
		return { };
	}
	return servlet;
}

ClusterClient::ClusterClient(const Poseidon::IpPort &addr, bool useSsl, boost::uint64_t keepAliveInterval,
	boost::int64_t mapX, boost::int64_t mapY)
	: Poseidon::Cbpp::Client(addr, useSsl, keepAliveInterval)
	, m_messageId(0), m_payload()
	, m_mapX(mapX), m_mapY(mapY)
	, m_serial(0)
{
	LOG_EMPERY_CLUSTER_INFO("ClusterClient constructor: this = ", (void *)this, ", addr = ", addr, ", useSsl = ", useSsl,
		", mapX = ", m_mapX, ", mapY = ", m_mapY);
}
ClusterClient::~ClusterClient(){
	LOG_EMPERY_CLUSTER_INFO("ClusterClient destructor: this = ", (void *)this,
		", mapX = ", m_mapX, ", mapY = ", m_mapY);
}

void ClusterClient::onClose(int errCode) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_INFO("Cluster client closed: errCode = ", errCode);

	try {
		Poseidon::enqueueAsyncJob(std::bind([&](const boost::shared_ptr<void> &){
			for(auto it = m_requests.begin(); it != m_requests.end(); ++it){
				const auto &promise = it->second.promise;
				if(!promise || promise->isSatisfied()){
					continue;
				}
				try {
					try {
						DEBUG_THROW(Exception, sslit("Lost connection to cluster server"));
					} catch(Poseidon::Exception &e){
						promise->setException(boost::copy_exception(e));
					} catch(std::exception &e){
						promise->setException(boost::copy_exception(e));
					}
				} catch(std::exception &e){
					LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			m_requests.clear();
		}, shared_from_this()));
	} catch(std::exception &e){
		LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
	}

	Poseidon::Cbpp::Client::onClose(errCode);
}

void ClusterClient::onSyncConnect(){
	PROFILE_ME;

	Poseidon::Cbpp::Client::onSyncConnect();

	Poseidon::enqueueAsyncJob(std::bind([&](const boost::shared_ptr<void> &){
		const auto result = sendAndWait(Msg::KS_MapRegisterCluster(m_mapX, m_mapY));
		if(result.first != 0){
			LOG_EMPERY_CLUSTER_ERROR("Failed to register cluster server: code = ", result.first, ", message = ", result.second);
			DEBUG_THROW(Exception, SharedNts(result.second));
		}
		LOG_EMPERY_CLUSTER_INFO("Cluster server registered successfully: mapX = ", m_mapX, ", mapY = ", m_mapY);
	}, shared_from_this()));
}
void ClusterClient::onSyncDataMessageHeader(boost::uint16_t messageId, boost::uint64_t payloadSize){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message header: messageId = ", messageId, ", payloadSize = ", payloadSize);

	m_messageId = messageId;
	m_payload.clear();
}
void ClusterClient::onSyncDataMessagePayload(boost::uint64_t payloadOffset, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message payload: payloadOffset = ", payloadOffset, ", payloadSize = ", payload.size());

	m_payload.splice(payload);
}
void ClusterClient::onSyncDataMessageEnd(boost::uint64_t payloadSize){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message end: payloadSize = ", payloadSize);

	auto messageId = m_messageId;
	auto payload = std::move(m_payload);
	LOG_EMPERY_CLUSTER_DEBUG("Received data message from center server: remote = ", getRemoteInfo(),
		", messageId = ", messageId, ", payloadSize = ", payload.size());

	if(messageId == Msg::G_PackedRequest::ID){
		Msg::G_PackedRequest packed(std::move(payload));
		Result result;
		try {
			const auto servlet = getServlet(packed.messageId);
			if(!servlet){
				LOG_EMPERY_CLUSTER_WARNING("No servlet found: messageId = ", packed.messageId);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown packed request"));
			}
			result = (*servlet)(virtualSharedFromThis<ClusterClient>(), Poseidon::StreamBuffer(packed.payload));
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_CLUSTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: messageId = ", messageId, ", what = ", e.what());
			result.first = e.statusCode();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: messageId = ", messageId, ", what = ", e.what());
			result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		LOG_EMPERY_CLUSTER_DEBUG("Sending response to center server: code = ", result.first, ", message = ", result.second);
		Poseidon::Cbpp::Client::send(Msg::G_PackedResponse(packed.serial, result.first, std::move(result.second)));
		if(result.first < 0){
			shutdownRead();
			shutdownWrite();
		}
	} else if(messageId == Msg::G_PackedResponse::ID){
		Msg::G_PackedResponse packed(std::move(payload));
		LOG_EMPERY_CLUSTER_DEBUG("Received response from center server: code = ", packed.code, ", message = ", packed.message);
		const auto it = m_requests.find(packed.serial);
		if(it != m_requests.end()){
			const auto elem = std::move(it->second);
			m_requests.erase(it);

			if(elem.result){
				*elem.result = std::make_pair(packed.code, std::move(packed.message));
			}
			if(elem.promise){
				elem.promise->setSuccess();
			}
		}
	} else {
		LOG_EMPERY_CLUSTER_WARNING("Unknown message from center server: remote = ", getRemoteInfo(), ", messageId = ", messageId);
		DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown message"));
	}
}

bool ClusterClient::send(boost::uint16_t messageId, Poseidon::StreamBuffer body){
	PROFILE_ME;

	const auto serial = ++m_serial;
	return Poseidon::Cbpp::Client::send(Msg::G_PackedRequest(serial, messageId, body.dump()));
}
void ClusterClient::shutdown(Poseidon::Cbpp::StatusCode errorCode, std::string errorMessage){
	PROFILE_ME;

	Poseidon::Cbpp::Client::sendControl(Poseidon::Cbpp::CTL_SHUTDOWN, errorCode, std::move(errorMessage));
	shutdownRead();
	shutdownWrite();
}

Result ClusterClient::sendAndWait(boost::uint16_t messageId, Poseidon::StreamBuffer body){
	PROFILE_ME;

	Result ret;

	const auto serial = ++m_serial;
	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	m_requests.insert(std::make_pair(serial, RequestElement(&ret, promise)));
	try {
		if(!Poseidon::Cbpp::Client::send(Msg::G_PackedRequest(serial, messageId, body.dump()))){
			DEBUG_THROW(Exception, sslit("Could not send data to center server"));
		}
		Poseidon::JobDispatcher::yield(promise);
		promise->checkAndRethrow();
	} catch(...){
		m_requests.erase(serial);
		throw;
	}
	m_requests.erase(serial);

	return ret;
}

bool ClusterClient::sendNotification(const EmperyCenter::AccountUuid &accountUuid, boost::uint16_t messageId, Poseidon::StreamBuffer body){
	PROFILE_ME;

	return Poseidon::Cbpp::Client::send(Msg::G_PackedAccountNotification(accountUuid.str(), messageId, body.dump()));
}

}
