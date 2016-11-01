#include "../precompiled.hpp"
#include "log_client.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/singletons/dns_daemon.hpp>
#include "../msg/kill.hpp"

namespace EmperyCenter {

namespace {
	struct {
		boost::shared_ptr<const Poseidon::JobPromise> promise;
		boost::shared_ptr<Poseidon::SockAddr> sock_addr;

		boost::weak_ptr<LogClient> log;
	} g_singleton;
}

boost::shared_ptr<LogClient> LogClient::get(){
	PROFILE_ME;

	auto log = g_singleton.log.lock();
	return log;
}
boost::shared_ptr<LogClient> LogClient::require(){
	PROFILE_ME;

	boost::shared_ptr<LogClient> log;
	for(;;){
		log = g_singleton.log.lock();
		if(log && !log->has_been_shutdown_write()){
			break;
		}
		const auto host       = get_config<std::string>   ("log_cbpp_client_host",         "127.0.0.1");
		const auto port       = get_config<unsigned>      ("log_cbpp_client_port",         13224);
		const auto use_ssl    = get_config<bool>          ("log_cbpp_client_use_ssl",      false);
		const auto keep_alive = get_config<std::uint64_t> ("log_cbpp_keep_alive_interval", 15000);
		LOG_EMPERY_CENTER_INFO("Creating log client: host = ", host, ", port = ", port, ", use_ssl = ", use_ssl);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!g_singleton.promise){
				auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
				auto promise = Poseidon::DnsDaemon::enqueue_for_looking_up(sock_addr, host, port);
				g_singleton.promise   = std::move(promise);
				g_singleton.sock_addr = std::move(sock_addr);
			}
			promise_tack = g_singleton.promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != g_singleton.promise);

		if(g_singleton.sock_addr){
			log.reset(new LogClient(*g_singleton.sock_addr, use_ssl, keep_alive));
			log->go_resident();

			g_singleton.promise.reset();
			g_singleton.sock_addr.reset();
			g_singleton.log = log;
		}
	}
	return log;
}

LogClient::LogClient(const Poseidon::SockAddr &sock_addr, bool use_ssl, std::uint64_t keep_alive_interval)
	: Poseidon::Cbpp::Client(sock_addr, use_ssl, keep_alive_interval)
{
	LOG_EMPERY_CENTER_INFO("Log client constructor: this = ", (void *)this);
}
LogClient::~LogClient(){
	LOG_EMPERY_CENTER_INFO("Log client destructor: this = ", (void *)this);
}

void LogClient::on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Received data message from log server: remote = ", get_remote_info(),
		", message_id = ", message_id, ", payload_size = ", payload.size());
}
void LogClient::on_sync_error_message(std::uint16_t message_id, Poseidon::Cbpp::StatusCode status_code, std::string reason){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Message response from log server: message_id = ", message_id,
		", status_code = ", status_code, ", reason = ", reason);
}

bool LogClient::send(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	return Poseidon::Cbpp::Client::send(message_id, std::move(payload));
}

void LogClient::shutdown(const char *message) noexcept {
	PROFILE_ME;

	shutdown(Poseidon::Cbpp::ST_INTERNAL_ERROR, message);
}
void LogClient::shutdown(int code, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		Poseidon::Cbpp::Client::send_control(Poseidon::Cbpp::CTL_SHUTDOWN, code, message);
		shutdown_read();
		shutdown_write();
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		force_shutdown();
	}
}

}
