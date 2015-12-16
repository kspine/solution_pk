#include "precompiled.hpp"
#include "cluster_session.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/async_job.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include "msg/g_packed.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

using Result          = ClusterSession::Result;
using ServletCallback = ClusterSession::ServletCallback;

namespace {
	boost::container::flat_map<unsigned, boost::weak_ptr<const ServletCallback> > g_servlet_map;
}

boost::shared_ptr<const ServletCallback> ClusterSession::create_servlet(boost::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[message_id];
	if(!weak.expired()){
		LOG_EMPERY_CENTER_ERROR("Duplicate cluster servlet: message_id = ", message_id);
		DEBUG_THROW(Exception, sslit("Duplicate cluster servlet"));
	}
	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> ClusterSession::get_servlet(boost::uint16_t message_id){
	PROFILE_ME;

	const auto it = g_servlet_map.find(message_id);
	if(it == g_servlet_map.end()){
		return { };
	}
	auto servlet = it->second.lock();
	if(!servlet){
		g_servlet_map.erase(it);
		return { };
	}
	return servlet;
}

ClusterSession::ClusterSession(Poseidon::UniqueFile socket)
	: Poseidon::Cbpp::Session(std::move(socket), 0x1000000) // 16MiB
	, m_serial(0)
{
	LOG_EMPERY_CENTER_INFO("Cluster session constructor: this = ", (void *)this);
}
ClusterSession::~ClusterSession(){
	LOG_EMPERY_CENTER_INFO("Cluster session destructor: this = ", (void *)this);
}

void ClusterSession::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CENTER_INFO("Cluster session closed: err_code = ", err_code);

	try {
		Poseidon::enqueue_async_job(std::bind([&](const boost::shared_ptr<void> &){
			for(auto it = m_requests.begin(); it != m_requests.end(); ++it){
				const auto &promise = it->second.promise;
				if(!promise || promise->is_satisfied()){
					continue;
				}
				try {
					try {
						DEBUG_THROW(Exception, sslit("Lost connection to cluster server"));
					} catch(Poseidon::Exception &e){
						promise->set_exception(boost::copy_exception(e));
					} catch(std::exception &e){
						promise->set_exception(boost::copy_exception(e));
					}
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			m_requests.clear();
		}, shared_from_this()));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	Poseidon::Cbpp::Session::on_close(err_code);
}

void ClusterSession::on_sync_data_message(boost::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Received data message from cluster server: remote = ", get_remote_info(),
		", message_id = ", message_id, ", size = ", payload.size());

	if(message_id == Msg::G_PackedRequest::ID){
		Msg::G_PackedRequest packed(std::move(payload));
		Result result;
		try {
			const auto servlet = get_servlet(packed.message_id);
			if(!servlet){
				LOG_EMPERY_CENTER_WARNING("No servlet found: message_id = ", packed.message_id);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown packed request"));
			}
			result = (*servlet)(virtual_shared_from_this<ClusterSession>(), Poseidon::StreamBuffer(packed.payload));
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = e.status_code();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		LOG_EMPERY_CENTER_TRACE("Sending response to cluster server: message_id = ", message_id,
			", code = ", result.first, ", message = ", result.second);
		Poseidon::Cbpp::Session::send(Msg::G_PackedResponse(packed.serial, result.first, std::move(result.second)));
		if(result.first < 0){
			shutdown_read();
			shutdown_write();
		}
	} else if(message_id == Msg::G_PackedResponse::ID){
		Msg::G_PackedResponse packed(std::move(payload));
		LOG_EMPERY_CENTER_TRACE("Received response from cluster server: code = ", packed.code, ", message = ", packed.message);
		const auto it = m_requests.find(packed.serial);
		if(it != m_requests.end()){
			const auto elem = std::move(it->second);
			m_requests.erase(it);

			if(elem.result){
				*elem.result = std::make_pair(packed.code, std::move(packed.message));
			}
			if(elem.promise){
				elem.promise->set_success();
			}
		}
	} else if(message_id == Msg::G_PackedAccountNotification::ID){
		Msg::G_PackedAccountNotification packed(std::move(payload));
		LOG_EMPERY_CENTER_TRACE("Forwarding message: account_uuid = ", packed.account_uuid,
			", message_id = ", packed.message_id, ", payload_size = ", packed.payload.size());
		const auto account_uuid = AccountUuid(packed.account_uuid);
		const auto session = PlayerSessionMap::get(account_uuid);
		if(!session){
			LOG_EMPERY_CENTER_TRACE("Player is not online: account_uuid = ", account_uuid);
		} else {
			try {
				session->send(packed.message_id, Poseidon::StreamBuffer(packed.payload));
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	} else {
		LOG_EMPERY_CENTER_WARNING("Unknown message from cluster server: remote = ", get_remote_info(), ", message_id = ", message_id);
		DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown message"));
	}
}

bool ClusterSession::send(boost::uint16_t message_id, Poseidon::StreamBuffer body){
	PROFILE_ME;

	const auto serial = ++m_serial;
	return Poseidon::Cbpp::Session::send(Msg::G_PackedRequest(serial, message_id, body.dump()));
}
void ClusterSession::shutdown(const char *message) noexcept {
	PROFILE_ME;

	shutdown(Poseidon::Cbpp::ST_INTERNAL_ERROR, message);
}
void ClusterSession::shutdown(int code, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		Poseidon::Cbpp::Session::send_error(Poseidon::Cbpp::ControlMessage::ID, code, message);
		shutdown_read();
		shutdown_write();
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		force_shutdown();
	}
}

Result ClusterSession::send_and_wait(boost::uint16_t message_id, Poseidon::StreamBuffer body){
	PROFILE_ME;

	Result ret;

	const auto serial = ++m_serial;
	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	m_requests.insert(std::make_pair(serial, RequestElement(&ret, promise)));
	try {
		if(!Poseidon::Cbpp::Session::send(Msg::G_PackedRequest(serial, message_id, body.dump()))){
			DEBUG_THROW(Exception, sslit("Could not send data to cluster server"));
		}
		Poseidon::JobDispatcher::yield(promise);
		promise->check_and_rethrow();
	} catch(...){
		m_requests.erase(serial);
		throw;
	}
	m_requests.erase(serial);

	return ret;
}

}
