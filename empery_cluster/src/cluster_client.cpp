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
	std::map<unsigned, boost::weak_ptr<const ServletCallback> > g_servlet_map;
}

boost::shared_ptr<const ServletCallback> ClusterClient::create_servlet(boost::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	const auto result = g_servlet_map.insert(std::make_pair(message_id, servlet));
	if(!result.second){
		if(!result.first->second.expired()){
			LOG_EMPERY_CLUSTER_ERROR("Duplicate cluster servlet: message_id = ", message_id);
			DEBUG_THROW(Exception, sslit("Duplicate cluster servlet"));
		}
		result.first->second = servlet;
	}
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> ClusterClient::get_servlet(boost::uint16_t message_id){
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

ClusterClient::ClusterClient(const Poseidon::IpPort &addr, bool use_ssl, boost::uint64_t keep_alive_interval,
	boost::int64_t map_x, boost::int64_t map_y)
	: Poseidon::Cbpp::Client(addr, use_ssl, keep_alive_interval)
	, m_message_id(0), m_payload()
	, m_map_x(map_x), m_map_y(map_y)
	, m_serial(0)
{
	LOG_EMPERY_CLUSTER_INFO("ClusterClient constructor: this = ", (void *)this, ", addr = ", addr, ", use_ssl = ", use_ssl,
		", map_x = ", m_map_x, ", map_y = ", m_map_y);
}
ClusterClient::~ClusterClient(){
	LOG_EMPERY_CLUSTER_INFO("ClusterClient destructor: this = ", (void *)this,
		", map_x = ", m_map_x, ", map_y = ", m_map_y);
}

void ClusterClient::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_INFO("Cluster client closed: err_code = ", err_code);

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
					LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			m_requests.clear();
		}, shared_from_this()));
	} catch(std::exception &e){
		LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
	}

	Poseidon::Cbpp::Client::on_close(err_code);
}

void ClusterClient::on_sync_connect(){
	PROFILE_ME;

	Poseidon::Cbpp::Client::on_sync_connect();

	Poseidon::enqueue_async_job(std::bind([&](const boost::shared_ptr<void> &){
		const auto result = send_and_wait(Msg::KS_MapRegisterCluster(m_map_x, m_map_y));
		if(result.first != 0){
			LOG_EMPERY_CLUSTER_ERROR("Failed to register cluster server: code = ", result.first, ", message = ", result.second);
			DEBUG_THROW(Exception, SharedNts(result.second));
		}
		LOG_EMPERY_CLUSTER_INFO("Cluster server registered successfully: map_x = ", m_map_x, ", map_y = ", m_map_y);
	}, shared_from_this()));
}
void ClusterClient::on_sync_data_message_header(boost::uint16_t message_id, boost::uint64_t payload_size){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message header: message_id = ", message_id, ", payload_size = ", payload_size);

	m_message_id = message_id;
	m_payload.clear();
}
void ClusterClient::on_sync_data_message_payload(boost::uint64_t payload_offset, Poseidon::StreamBuffer payload){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message payload: payload_offset = ", payload_offset, ", payload_size = ", payload.size());

	m_payload.splice(payload);
}
void ClusterClient::on_sync_data_message_end(boost::uint64_t payload_size){
	PROFILE_ME;
	LOG_EMPERY_CLUSTER_TRACE("Message end: payload_size = ", payload_size);

	auto message_id = m_message_id;
	auto payload = std::move(m_payload);
	LOG_EMPERY_CLUSTER_DEBUG("Received data message from center server: remote = ", get_remote_info(),
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
				"Poseidon::Cbpp::Exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = e.status_code();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		LOG_EMPERY_CLUSTER_DEBUG("Sending response to center server: code = ", result.first, ", message = ", result.second);
		Poseidon::Cbpp::Client::send(Msg::G_PackedResponse(packed.serial, result.first, std::move(result.second)));
		if(result.first < 0){
			shutdown_read();
			shutdown_write();
		}
	} else if(message_id == Msg::G_PackedResponse::ID){
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
				elem.promise->set_success();
			}
		}
	} else {
		LOG_EMPERY_CLUSTER_WARNING("Unknown message from center server: remote = ", get_remote_info(), ", message_id = ", message_id);
		DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND, sslit("Unknown message"));
	}
}

bool ClusterClient::send(boost::uint16_t message_id, Poseidon::StreamBuffer body){
	PROFILE_ME;

	const auto serial = ++m_serial;
	return Poseidon::Cbpp::Client::send(Msg::G_PackedRequest(serial, message_id, body.dump()));
}
void ClusterClient::shutdown(Poseidon::Cbpp::StatusCode error_code, std::string error_message){
	PROFILE_ME;

	Poseidon::Cbpp::Client::send_control(Poseidon::Cbpp::CTL_SHUTDOWN, error_code, std::move(error_message));
	shutdown_read();
	shutdown_write();
}

Result ClusterClient::send_and_wait(boost::uint16_t message_id, Poseidon::StreamBuffer body){
	PROFILE_ME;

	Result ret;

	const auto serial = ++m_serial;
	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	m_requests.insert(std::make_pair(serial, RequestElement(&ret, promise)));
	try {
		if(!Poseidon::Cbpp::Client::send(Msg::G_PackedRequest(serial, message_id, body.dump()))){
			DEBUG_THROW(Exception, sslit("Could not send data to center server"));
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

bool ClusterClient::send_notification(const EmperyCenter::AccountUuid &account_uuid, boost::uint16_t message_id, Poseidon::StreamBuffer body){
	PROFILE_ME;

	return Poseidon::Cbpp::Client::send(Msg::G_PackedAccountNotification(account_uuid.str(), message_id, body.dump()));
}

}
