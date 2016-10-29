#include "precompiled.hpp"
#include "log_session.hpp"
#include "mmain.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/cbpp/exception.hpp>

namespace EmperyCenterLog {

using ServletCallback = LogSession::ServletCallback;

namespace {
	boost::container::flat_map<unsigned, boost::weak_ptr<const ServletCallback>> g_servlet_map;
}

boost::shared_ptr<const ServletCallback> LogSession::create_servlet(std::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[message_id];
	if(!weak.expired()){
		LOG_EMPERY_CENTER_LOG_ERROR("Duplicate log servlet: message_id = ", message_id);
		DEBUG_THROW(Exception, sslit("Duplicate log servlet"));
	}
	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> LogSession::get_servlet(std::uint16_t message_id){
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

LogSession::LogSession(Poseidon::UniqueFile socket)
	: Poseidon::Cbpp::Session(std::move(socket),
		get_config<std::uint64_t>("log_session_max_packet_size", 0x1000000))
{
	LOG_EMPERY_CENTER_LOG_INFO("Log session constructor: this = ", (void *)this);
}
LogSession::~LogSession(){
	LOG_EMPERY_CENTER_LOG_INFO("Log session destructor: this = ", (void *)this);
}

void LogSession::on_connect(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_LOG_INFO("Log session connected: remote = ", get_remote_info());

	const auto initial_timeout = get_config<std::uint64_t>("log_session_initial_timeout", 30000);
	set_timeout(initial_timeout);

	Poseidon::Cbpp::Session::on_connect();
}

void LogSession::on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CENTER_LOG_TRACE("Received data message from log client: remote = ", get_remote_info(),
		", message_id = ", message_id, ", size = ", payload.size());

	auto result = std::pair<long, std::string>(Poseidon::Cbpp::ST_OK, std::string());
	try {
		const auto servlet = get_servlet(message_id);
		if(!servlet){
			LOG_EMPERY_CENTER_LOG_WARNING("No servlet found: message_id = ", message_id);
			DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown packed request"));
		}
		(*servlet)(virtual_shared_from_this<LogSession>(), std::move(payload));
	} catch(Poseidon::Cbpp::Exception &e){
		LOG_EMPERY_CENTER_LOG(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
			"Poseidon::Cbpp::Exception thrown: message_id = ", message_id, ", what = ", e.what());
		result.first = e.get_status_code();
		result.second = e.what();
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_LOG(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
			"std::exception thrown: message_id = ", message_id, ", what = ", e.what());
		result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
		result.second = e.what();
	}
	if(result.first != 0){
		LOG_EMPERY_CENTER_LOG_DEBUG("Sending response to log client: message_id = ", message_id,
			", code = ", result.first, ", message = ", result.second);
	}

	Poseidon::Cbpp::Session::send_error(0, result.first, result.second.c_str());
}
void LogSession::on_sync_control_message(Poseidon::Cbpp::ControlCode control_code, std::int64_t vint_param, std::string string_param){
	PROFILE_ME;
	LOG_EMPERY_CENTER_LOG_TRACE("Control message from log client: control_code = ", control_code,
		", vint_param = ", vint_param, ", string_param = ", string_param);

	Poseidon::Cbpp::Session::on_sync_control_message(control_code, vint_param, std::move(string_param));
}

void LogSession::shutdown(const char *message) noexcept {
	PROFILE_ME;

	shutdown(Poseidon::Cbpp::ST_INTERNAL_ERROR, message);
}
void LogSession::shutdown(int code, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		Poseidon::Cbpp::Session::send_error(0, code, message);
		shutdown_read();
		shutdown_write();
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_LOG_ERROR("std::exception thrown: what = ", e.what());
		force_shutdown();
	}
}

}
