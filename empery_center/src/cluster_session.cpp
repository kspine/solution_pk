#include "precompiled.hpp"
#include "cluster_session.hpp"
#include "mmain.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/atomic.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include "msg/g_packed.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "singletons/world_map.hpp"

namespace EmperyCenter {

using Result          = ClusterSession::Result;
using ServletCallback = ClusterSession::ServletCallback;

namespace {
	boost::container::flat_map<unsigned, boost::weak_ptr<const ServletCallback>> g_servlet_map;
}

boost::shared_ptr<const ServletCallback> ClusterSession::create_servlet(std::uint16_t message_id, ServletCallback callback){
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
boost::shared_ptr<const ServletCallback> ClusterSession::get_servlet(std::uint16_t message_id){
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
	: Poseidon::Cbpp::Session(std::move(socket),
		get_config<std::uint64_t>("cluster_session_max_packet_size", 0x1000000))
	, m_serial(0)
{
	LOG_EMPERY_CENTER_INFO("Cluster session constructor: this = ", (void *)this);
}
ClusterSession::~ClusterSession(){
	LOG_EMPERY_CENTER_INFO("Cluster session destructor: this = ", (void *)this);
}

void ClusterSession::on_connect(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_INFO("Cluster session connected: remote = ", get_remote_info());

	const auto initial_timeout = get_config<std::uint64_t>("cluster_session_initial_timeout", 30000);
	set_timeout(initial_timeout);

	Poseidon::Cbpp::Session::on_connect();
}
void ClusterSession::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CENTER_INFO("Cluster session closed: err_code = ", err_code);

	{
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);

		const auto requests = std::move(m_requests);
		m_requests.clear();
		for(auto it = requests.begin(); it != requests.end(); ++it){
			const auto &promise = it->second.promise;
			if(!promise || promise->is_satisfied()){
				continue;
			}
			try {
				try {
					DEBUG_THROW(Exception, sslit("Lost connection to cluster client"));
				} catch(Poseidon::Exception &e){
					promise->set_exception(boost::copy_exception(e));
				} catch(std::exception &e){
					promise->set_exception(boost::copy_exception(e));
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	Poseidon::Cbpp::Session::on_close(err_code);
}

bool ClusterSession::on_low_level_data_message_end(std::uint64_t payload_size){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Received data message from cluster client: remote = ", get_remote_info(),
		", message_id = ", get_low_level_message_id(), ", size = ", payload_size);

	const auto message_id = get_low_level_message_id();
	if(message_id == Msg::G_PackedResponse::ID){
		Msg::G_PackedResponse packed(get_low_level_payload());

		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
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
	}

	return Poseidon::Cbpp::Session::on_low_level_data_message_end(payload_size);
}

void ClusterSession::on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Received data message from cluster client: remote = ", get_remote_info(),
		", message_id = ", message_id, ", size = ", payload.size());

	if(message_id != Msg::G_PackedResponse::ID){
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
					"Poseidon::Cbpp::Exception thrown: message_id = ", packed.message_id, ", what = ", e.what());
				result.first = e.status_code();
				result.second = e.what();
			} catch(std::exception &e){
				LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
					"std::exception thrown: message_id = ", packed.message_id, ", what = ", e.what());
				result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
				result.second = e.what();
			}
			if(result.first != 0){
				LOG_EMPERY_CENTER_DEBUG("Sending response to cluster server: message_id = ", packed.message_id,
					", code = ", result.first, ", message = ", result.second);
			}

			Msg::G_PackedResponse res;
			res.serial  = packed.serial;
			res.code    = result.first;
			res.message = std::move(result.second);
			Poseidon::Cbpp::Session::send(res.ID, Poseidon::StreamBuffer(res));

			if(result.first < 0){
				shutdown_read();
				shutdown_write();
			}
		} else if(message_id == Msg::G_PackedAccountNotification::ID){
			Msg::G_PackedAccountNotification packed(std::move(payload));

			const auto account_uuid = AccountUuid(packed.account_uuid);
			LOG_EMPERY_CENTER_TRACE("Forwarding message: account_uuid = ", account_uuid,
				", message_id = ", packed.message_id, ", payload_size = ", packed.payload.size());
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
		} else if(message_id == Msg::G_PackedRectangleNotification::ID){
			Msg::G_PackedRectangleNotification packed(std::move(payload));

			const auto rectangle = Rectangle(packed.x, packed.y, packed.width, packed.height);
			LOG_EMPERY_CENTER_TRACE("Forwarding message: rectangle = ", rectangle,
				", message_id = ", packed.message_id, ", payload_size = ", packed.payload.size());
			std::vector<boost::shared_ptr<PlayerSession>> sessions;
			WorldMap::get_players_viewing_rectangle(sessions, rectangle);
			for(auto it = sessions.begin(); it != sessions.end(); ++it){
				const auto &session = *it;
				try {
					session->send(packed.message_id, Poseidon::StreamBuffer(packed.payload));
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					session->shutdown(e.what());
				}
			}
		} else {
			LOG_EMPERY_CENTER_WARNING("Unknown message from cluster client: remote = ", get_remote_info(), ", message_id = ", message_id);
			DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown message"));
		}
	}
}
void ClusterSession::on_sync_control_message(Poseidon::Cbpp::ControlCode control_code, std::int64_t vint_param, std::string string_param){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Control message from cluster client: control_code = ", control_code,
		", vint_param = ", vint_param, ", string_param = ", string_param);

	Poseidon::Cbpp::Session::on_sync_control_message(control_code, vint_param, std::move(string_param));
}

bool ClusterSession::send(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	const auto serial = Poseidon::atomic_add(m_serial, 1, Poseidon::ATOMIC_RELAXED);
	Msg::G_PackedRequest msg;
	msg.serial     = serial;
	msg.message_id = message_id;
	msg.payload    = payload.dump();
	return Poseidon::Cbpp::Session::send(msg.ID, Poseidon::StreamBuffer(msg));
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

Result ClusterSession::send_and_wait(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	Result ret;

	const auto serial = Poseidon::atomic_add(m_serial, 1, Poseidon::ATOMIC_RELAXED);
	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	{
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		m_requests.emplace(serial, RequestElement(&ret, promise));
	}
	try {
		Msg::G_PackedRequest msg;
		msg.serial     = serial;
		msg.message_id = message_id;
		msg.payload    = payload.dump();
		if(!Poseidon::Cbpp::Session::send(msg.ID, Poseidon::StreamBuffer(msg))){
			DEBUG_THROW(Exception, sslit("Could not send data to cluster client"));
		}
		Poseidon::JobDispatcher::yield(promise, true);
	} catch(...){
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		m_requests.erase(serial);
		throw;
	}
	{
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		m_requests.erase(serial);
	}

	return ret;
}

}
