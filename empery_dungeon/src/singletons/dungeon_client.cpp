#include "../precompiled.hpp"
#include "dungeon_client.hpp"
#include "../mmain.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/atomic.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/singletons/dns_daemon.hpp>
#include "../../../empery_center/src/msg/g_packed.hpp"

namespace EmperyDungeon {

namespace Msg = ::EmperyCenter::Msg;

using Result          = DungeonClient::Result;
using ServletCallback = DungeonClient::ServletCallback;

namespace {
	boost::container::flat_map<unsigned, boost::weak_ptr<const ServletCallback>> g_servlet_map;

	struct {
		boost::shared_ptr<const Poseidon::JobPromise> promise;
		boost::shared_ptr<Poseidon::SockAddr> sock_addr;

		boost::weak_ptr<DungeonClient> client;
	} g_singleton;
}

boost::shared_ptr<const ServletCallback> DungeonClient::create_servlet(std::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[message_id];
	if(!weak.expired()){
		LOG_EMPERY_DUNGEON_ERROR("Duplicate dungeon servlet: message_id = ", message_id);
		DEBUG_THROW(Exception, sslit("Duplicate dungeon servlet"));
	}
	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> DungeonClient::get_servlet(std::uint16_t message_id){
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

boost::shared_ptr<DungeonClient> DungeonClient::get(){
	PROFILE_ME;

	auto client = g_singleton.client.lock();
	return client;
}
boost::shared_ptr<DungeonClient> DungeonClient::require(){
	PROFILE_ME;

	boost::shared_ptr<DungeonClient> client;
	for(;;){
		client = g_singleton.client.lock();
		if(client && !client->has_been_shutdown_write()){
			break;
		}
		const auto host       = get_config<std::string>   ("dungeon_cbpp_client_host",         "127.0.0.1");
		const auto port       = get_config<unsigned>      ("dungeon_cbpp_client_port",         13225);
		const auto use_ssl    = get_config<bool>          ("dungeon_cbpp_client_use_ssl",      false);
		const auto keep_alive = get_config<std::uint64_t> ("dungeon_cbpp_keep_alive_interval", 15000);
		LOG_EMPERY_DUNGEON_INFO("Creating dungeon client: host = ", host, ", port = ", port, ", use_ssl = ", use_ssl);

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
			client.reset(new DungeonClient(*g_singleton.sock_addr, use_ssl, keep_alive));
			client->go_resident();

			g_singleton.promise.reset();
			g_singleton.sock_addr.reset();
			g_singleton.client = client;
		}
	}
	return client;
}

DungeonClient::DungeonClient(const Poseidon::SockAddr &sock_addr, bool use_ssl, std::uint64_t keep_alive_interval)
	: Poseidon::Cbpp::Client(sock_addr, use_ssl, keep_alive_interval)
	, m_serial(0)
{
	LOG_EMPERY_DUNGEON_INFO("Dungeon client constructor: this = ", (void *)this);
}
DungeonClient::~DungeonClient(){
	LOG_EMPERY_DUNGEON_INFO("Dungeon client destructor: this = ", (void *)this);
}

void DungeonClient::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_DUNGEON_INFO("Dungeon client closed: err_code = ", err_code);

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
					DEBUG_THROW(Exception, sslit("Lost connection to center server"));
				} catch(Poseidon::Exception &e){
					promise->set_exception(boost::copy_exception(e));
				} catch(std::exception &e){
					promise->set_exception(boost::copy_exception(e));
				}
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	Poseidon::Cbpp::Client::on_close(err_code);
}

bool DungeonClient::on_low_level_data_message_end(std::uint64_t payload_size){
	PROFILE_ME;
	LOG_EMPERY_DUNGEON_TRACE("Received data message from center server: remote = ", get_remote_info(),
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

	return Poseidon::Cbpp::Client::on_low_level_data_message_end(payload_size);
}

void DungeonClient::on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_DUNGEON_TRACE("Received data message from center server: remote = ", get_remote_info(),
		", message_id = ", message_id, ", payload_size = ", payload.size());

	if(message_id == Msg::G_PackedRequest::ID){
		Msg::G_PackedRequest packed(std::move(payload));

		Result result;
		try {
			const auto servlet = get_servlet(packed.message_id);
			if(!servlet){
				LOG_EMPERY_DUNGEON_WARNING("No servlet found: message_id = ", packed.message_id);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown packed request"));
			}
			result = (*servlet)(virtual_shared_from_this<DungeonClient>(), Poseidon::StreamBuffer(packed.payload));
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_DUNGEON(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: message_id = ", packed.message_id, ", what = ", e.what());
			result.first = e.status_code();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_DUNGEON(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: message_id = ", packed.message_id, ", what = ", e.what());
			result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		if(result.first != 0){
			LOG_EMPERY_DUNGEON_DEBUG("Sending response to center server: message_id = ", packed.message_id,
				", code = ", result.first, ", message = ", result.second);
		}

		Msg::G_PackedResponse res;
		res.serial  = packed.serial;
		res.code    = result.first;
		res.message = std::move(result.second);
		Poseidon::Cbpp::Client::send(res.ID, Poseidon::StreamBuffer(res));

		if(result.first < 0){
			shutdown_read();
			shutdown_write();
		}
	} else if(message_id == Msg::G_PackedResponse::ID){
		Msg::G_PackedResponse packed(std::move(payload));

		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		const auto it = m_requests.find(packed.serial);
		if(it != m_requests.end()){
			const auto elem = std::move(it->second);
			m_requests.erase(it);

			if(elem.result){
				elem.result->first  = packed.code;
				elem.result->second = std::move(packed.message);
			}
			if(elem.promise){
				elem.promise->set_success();
			}
		}
	} else {
		LOG_EMPERY_DUNGEON_WARNING("Unknown message from center server: remote = ", get_remote_info(), ", message_id = ", message_id);
		DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown message"));
	}
}

void DungeonClient::on_sync_error_message(std::uint16_t message_id, Poseidon::Cbpp::StatusCode status_code, std::string reason){
	PROFILE_ME;
	LOG_EMPERY_DUNGEON_TRACE("Message response from center server: message_id = ", message_id,
		", status_code = ", status_code, ", reason = ", reason);
}

bool DungeonClient::send(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	const auto serial = Poseidon::atomic_add(m_serial, 1, Poseidon::ATOMIC_RELAXED);
	Msg::G_PackedRequest msg;
	msg.serial     = serial;
	msg.message_id = message_id;
	msg.payload    = payload.dump();
	return Poseidon::Cbpp::Client::send(msg.ID, Poseidon::StreamBuffer(msg));
}

void DungeonClient::shutdown(const char *message) noexcept {
	PROFILE_ME;

	shutdown(Poseidon::Cbpp::ST_INTERNAL_ERROR, message);
}
void DungeonClient::shutdown(int code, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		Poseidon::Cbpp::Client::send_control(Poseidon::Cbpp::CTL_SHUTDOWN, code, message);
		shutdown_read();
		shutdown_write();
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_ERROR("std::exception thrown: what = ", e.what());
		force_shutdown();
	}
}

Result DungeonClient::send_and_wait(std::uint16_t message_id, Poseidon::StreamBuffer payload){
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
		if(!Poseidon::Cbpp::Client::send(msg.ID, Poseidon::StreamBuffer(msg))){
			DEBUG_THROW(Exception, sslit("Could not send data to center server"));
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
