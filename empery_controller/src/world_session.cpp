#include "precompiled.hpp"
#include "world_session.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/json.hpp>

namespace EmperyController {

using ServletCallback = WorldSession::ServletCallback;

namespace {
	boost::container::flat_map<std::string, boost::weak_ptr<const ServletCallback>> g_servlet_map;
}

WorldSession::WorldSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_prefix(std::move(prefix))
{
}
WorldSession::~WorldSession(){
}

boost::shared_ptr<const ServletCallback> WorldSession::create_servlet(const std::string &uri, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[uri];
	if(!weak.expired()){
		LOG_EMPERY_CONTROLLER_ERROR("Duplicate world HTTP servlet: uri = ", uri);
		DEBUG_THROW(Exception, sslit("Duplicate world HTTP servlet"));
	}
	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> WorldSession::get_servlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_servlet_map.find(uri);
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

void WorldSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_CONTROLLER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted world HTTP request from ", get_remote_info());

	Poseidon::OptionalMap headers;
	headers.set(sslit("Access-Control-Allow-Origin"),  "*");
	headers.set(sslit("Access-Control-Allow-Headers"), "Authorization");
	if(request_headers.verb == Poseidon::Http::V_OPTIONS){
		send(Poseidon::Http::ST_OK, std::move(headers));
		return;
	}

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_CONTROLLER_WARNING("Inacceptable world HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = get_servlet(uri);
	if(!servlet){
		LOG_EMPERY_CONTROLLER_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	std::pair<long, std::string> result;
	Poseidon::JsonObject root;
	try {
		result = (*servlet)(root, virtual_shared_from_this<WorldSession>(), std::move(request_headers.get_params));
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(Poseidon::Cbpp::Exception &e){
		LOG_EMPERY_CONTROLLER_WARNING("Poseidon::Cbpp::Exception thrown: what = ", e.what());
		result.first = e.get_status_code();
		result.second = e.what();
	} catch(std::exception &e){
		LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
		result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
		result.second = e.what();
	}
	root[sslit("err_code")] = result.first;
	root[sslit("err_msg")] = std::move(result.second);
	LOG_EMPERY_CONTROLLER_DEBUG("Sending response: ", root.dump());
	headers.set(sslit("Content-Type"), "application/json");
	send(Poseidon::Http::ST_OK, std::move(headers), Poseidon::StreamBuffer(root.dump()));
}

}
