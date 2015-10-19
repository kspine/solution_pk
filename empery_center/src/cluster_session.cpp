#include "precompiled.hpp"
#include "cluster_session.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include "msg/ss_packed.hpp"

namespace EmperyCenter {

using Result          = ClusterSession::Result;
using ServletCallback = ClusterSession::ServletCallback;

namespace {
	std::map<unsigned, boost::weak_ptr<const ServletCallback> > g_servletMap;
}

boost::shared_ptr<const ServletCallback> ClusterSession::createServlet(boost::uint16_t messageId, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	const auto result = g_servletMap.insert(std::make_pair(messageId, servlet));
	if(!result.second){
		if(!result.first->second.expired()){
			LOG_EMPERY_CENTER_ERROR("Duplicate cluster servlet: messageId = ", messageId);
			DEBUG_THROW(Exception, sslit("Duplicate cluster servlet"));
		}
		result.first->second = servlet;
	}
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> ClusterSession::getServlet(boost::uint16_t messageId){
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

ClusterSession::ClusterSession(Poseidon::UniqueFile socket)
	: Poseidon::Cbpp::Session(std::move(socket))
	, m_serial(0)
{
	LOG_EMPERY_CENTER_INFO("ClusterSession constructor: this = ", (void *)this);
}
ClusterSession::~ClusterSession(){
	LOG_EMPERY_CENTER_INFO("ClusterSession destructor: this = ", (void *)this);
}

void ClusterSession::onClose(int errCode) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CENTER_INFO("Cluster session closed: errCode = ", errCode);

	for(auto it = m_requests.begin(); it != m_requests.end(); ++it){
		const auto &promise = it->second.promise;
		if(!promise){
			continue;
		}
		try {
			DEBUG_THROW(Exception, sslit("Lost connection to cluster server"));
		} catch(Poseidon::Exception &e){
			LOG_EMPERY_CENTER_WARNING("Poseidon::Exception thrown: what = ", e.what());
			promise->setException(boost::copy_exception(e));
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			promise->setException(boost::copy_exception(e));
		}
	}

	Poseidon::Cbpp::Session::onClose(errCode);
}

void ClusterSession::onSyncDataMessage(boost::uint16_t messageId, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CENTER_DEBUG("Received data message from cluster server: remote = ", getRemoteInfo(),
		", messageId = ", messageId, ", size = ", payload.size());

	if(messageId == Msg::SS_PackedRequest::ID){
		Msg::SS_PackedRequest packed(std::move(payload));
		Result result;
		try {
			const auto servlet = getServlet(packed.messageId);
			if(!servlet){
				LOG_EMPERY_CENTER_WARNING("No servlet found: messageId = ", packed.messageId);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown packed request"));
			}
			result = (*servlet)(virtualSharedFromThis<ClusterSession>(), Poseidon::StreamBuffer(packed.payload));
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: messageId = ", messageId, ", what = ", e.what());
			Poseidon::Cbpp::Session::send(Msg::SS_PackedResponse(packed.serial, e.statusCode(), e.what()));
		} catch(std::exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: messageId = ", messageId, ", what = ", e.what());
			Poseidon::Cbpp::Session::send(Msg::SS_PackedResponse(packed.serial, Poseidon::Cbpp::ST_INTERNAL_ERROR, e.what()));
		}
		LOG_EMPERY_CENTER_DEBUG("Sending response to cluster server: code = ", result.first, ", message = ", result.second);
		Poseidon::Cbpp::Session::send(Msg::SS_PackedResponse(packed.serial, result.first, std::move(result.second)));
	} else if(messageId == Msg::SS_PackedResponse::ID){
		Msg::SS_PackedResponse packed(std::move(payload));
		LOG_EMPERY_CENTER_DEBUG("Received response from cluster server: code = ", packed.code, ", message = ", packed.message);
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
		LOG_EMPERY_CENTER_WARNING("Unknown message from cluster server: remote = ", getRemoteInfo(), ", messageId = ", messageId);
		DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown message"));
	}
}

bool ClusterSession::send(boost::uint16_t messageId, Poseidon::StreamBuffer body){
	PROFILE_ME;

	const auto serial = ++m_serial;
	return Poseidon::Cbpp::Session::send(Msg::SS_PackedRequest(serial, messageId, body.dump()));
}
void ClusterSession::shutdown(Poseidon::Cbpp::StatusCode errorCode, std::string errorMessage){
	PROFILE_ME;

	Poseidon::Cbpp::Session::sendError(Poseidon::Cbpp::ControlMessage::ID, errorCode, std::move(errorMessage));
	shutdownRead();
	shutdownWrite();
}

Result ClusterSession::sendAndWait(boost::uint16_t messageId, Poseidon::StreamBuffer body){
	PROFILE_ME;

	Result ret;

	const auto serial = ++m_serial;
	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	m_requests.insert(std::make_pair(serial, RequestElement(&ret, promise)));
	try {
		if(!Poseidon::Cbpp::Session::send(Msg::SS_PackedRequest(serial, messageId, body.dump()))){
			DEBUG_THROW(Exception, sslit("Could not send data to cluster server"));
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

}
