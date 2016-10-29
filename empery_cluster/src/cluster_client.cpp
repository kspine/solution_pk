#include "precompiled.hpp"
#include "cluster_client.hpp"
#include "mmain.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/singletons/dns_daemon.hpp>
#include "../../empery_center/src/msg/g_packed.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"

namespace EmperyCluster {

namespace Msg = ::EmperyCenter::Msg;

using Result          = ClusterClient::Result;
using ServletCallback = ClusterClient::ServletCallback;

namespace {
	boost::container::flat_map<unsigned, boost::weak_ptr<const ServletCallback>> g_servlet_map;
}

boost::shared_ptr<const ServletCallback> ClusterClient::create_servlet(std::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[message_id];
	if(!weak.expired()){
		LOG_EMPERY_CLUSTER_ERROR("Duplicate cluster servlet: message_id = ", message_id);
		DEBUG_THROW(Exception, sslit("Duplicate cluster servlet"));
	}
	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> ClusterClient::get_servlet(std::uint16_t message_id){
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

boost::shared_ptr<ClusterClient> ClusterClient::create(std::int64_t numerical_x, std::int64_t numerical_y,
	std::string name, std::uint64_t created_time)
{
	PROFILE_ME;

	const auto host       = get_config<std::string>   ("cluster_cbpp_client_host",         "127.0.0.1");
	const auto port       = get_config<unsigned>      ("cluster_cbpp_client_port",         13227);
	const auto use_ssl    = get_config<bool>          ("cluster_cbpp_client_use_ssl",      false);
	const auto keep_alive = get_config<std::uint64_t> ("cluster_cbpp_keep_alive_interval", 15000);

	const auto sock_addr = boost::make_shared<Poseidon::SockAddr>();

	const auto promise = Poseidon::DnsDaemon::enqueue_for_looking_up(sock_addr, host, port);
	Poseidon::JobDispatcher::yield(promise, true);
	LOG_EMPERY_CLUSTER_DEBUG("DNS lookup succeeded: host = ", host, ", ip = ", Poseidon::get_ip_port_from_sock_addr(*sock_addr).ip);

	auto client = boost::shared_ptr<ClusterClient>(new ClusterClient(*sock_addr, use_ssl, keep_alive));
	client->go_resident();
	try {
		Msg::KS_MapRegisterCluster msg;
		msg.numerical_x  = numerical_x;
		msg.numerical_y  = numerical_y;
		msg.name         = std::move(name);
		msg.created_time = created_time;
		if(!client->send(msg)){
			LOG_EMPERY_CLUSTER_ERROR("Failed to send data to cluster server!");
			DEBUG_THROW(Exception, sslit("Failed to send data to cluster server"));
		}
	} catch(std::exception &e){
		LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
		client->force_shutdown();
		throw;
	}

	return client;
}

ClusterClient::ClusterClient(const Poseidon::SockAddr &sock_addr, bool use_ssl, std::uint64_t keep_alive_interval)
	: Poseidon::Cbpp::Client(sock_addr, use_ssl, keep_alive_interval)
{
	LOG_EMPERY_CLUSTER_INFO("Cluster client constructor: this = ", (void *)this);
}
ClusterClient::~ClusterClient(){
	LOG_EMPERY_CLUSTER_INFO("Cluster client destructor: this = ", (void *)this);
}

void ClusterClient::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_INFO("Cluster client closed: err_code = ", err_code);

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
					DEBUG_THROW(Exception, sslit("Lost connection to cluster server"));
				} catch(Poseidon::Exception &e){
					promise->set_exception(boost::copy_exception(e));
				} catch(std::exception &e){
					promise->set_exception(boost::copy_exception(e));
				}
			} catch(std::exception &e){
				LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	Poseidon::Cbpp::Client::on_close(err_code);
}

bool ClusterClient::on_low_level_data_message_end(std::uint64_t payload_size){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Received data message from center server: remote = ", get_remote_info(),
		", message_id = ", get_low_level_message_id(), ", size = ", payload_size);

	const auto message_id = get_low_level_message_id();
	if(message_id == Msg::G_PackedResponse::ID){
		Msg::G_PackedResponse packed(get_low_level_payload());
		if(!packed.uuid.empty()){
			const auto uuid = Poseidon::Uuid(packed.uuid);

			const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
			const auto it = m_requests.find(uuid);
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
	}

	return Poseidon::Cbpp::Client::on_low_level_data_message_end(payload_size);
}

void ClusterClient::on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Received data message from center server: remote = ", get_remote_info(),
		", message_id = ", message_id, ", payload_size = ", payload.size());

	if(message_id == Msg::G_PackedRequest::ID){
		Msg::G_PackedRequest packed(std::move(payload));

		Result result;
		try {
			const auto servlet = get_servlet(packed.message_id);
			if(!servlet){
				LOG_EMPERY_CLUSTER_WARNING("No servlet found: message_id = ", packed.message_id);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown packed request"));
			}
			result = (*servlet)(virtual_shared_from_this<ClusterClient>(), Poseidon::StreamBuffer(packed.payload));
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_CLUSTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: message_id = ", packed.message_id, ", what = ", e.what());
			result.first = e.get_status_code();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: message_id = ", packed.message_id, ", what = ", e.what());
			result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		if(result.first != 0){
			LOG_EMPERY_CLUSTER_DEBUG("Sending response to center server: message_id = ", packed.message_id,
				", code = ", result.first, ", message = ", result.second);
		}

		Msg::G_PackedResponse res;
		res.uuid    = std::move(packed.uuid);
		res.code    = result.first;
		res.message = std::move(result.second);
		Poseidon::Cbpp::Client::send(res.ID, Poseidon::StreamBuffer(res));

		if(result.first < 0){
			shutdown_read();
			shutdown_write();
		}
	}
}

void ClusterClient::on_sync_error_message(std::uint16_t message_id, Poseidon::Cbpp::StatusCode status_code, std::string reason){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message response from center server: message_id = ", message_id,
		", status_code = ", status_code, ", reason = ", reason);
}

bool ClusterClient::send(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	Msg::G_PackedRequest msg;
	msg.message_id = message_id;
	msg.payload    = payload.dump();
	return Poseidon::Cbpp::Client::send(msg.ID, Poseidon::StreamBuffer(msg));
}

void ClusterClient::shutdown(const char *message) noexcept {
	PROFILE_ME;

	shutdown(Poseidon::Cbpp::ST_INTERNAL_ERROR, message);
}
void ClusterClient::shutdown(int code, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		Poseidon::Cbpp::Client::send_control(Poseidon::Cbpp::CTL_SHUTDOWN, code, message);
		shutdown_read();
		shutdown_write();
	} catch(std::exception &e){
		LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
		force_shutdown();
	}
}

Result ClusterClient::send_and_wait(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	const auto ret = boost::make_shared<Result>();
	const auto uuid = Poseidon::Uuid::random();
	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	{
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		m_requests.emplace(uuid, RequestElement(ret, promise));
	}
	try {
		Msg::G_PackedRequest msg;
		msg.uuid       = uuid.to_string();
		msg.message_id = message_id;
		msg.payload    = payload.dump();
		if(!Poseidon::Cbpp::Client::send(msg.ID, Poseidon::StreamBuffer(msg))){
			DEBUG_THROW(Exception, sslit("Could not send data to center server"));
		}
		Poseidon::JobDispatcher::yield(promise, true);
	} catch(...){
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		m_requests.erase(uuid);
		throw;
	}
	{
		const Poseidon::Mutex::UniqueLock lock(m_request_mutex);
		m_requests.erase(uuid);
	}
	return std::move(*ret);
}

}
