#include "precompiled.hpp"
#include "account_http_session.hpp"
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/json.hpp>

namespace EmperyGateWestwalk {

using ServletCallback = AccountHttpSession::ServletCallback;

namespace {
	std::map<std::string, boost::weak_ptr<const ServletCallback> > g_servlet_map;
}

AccountHttpSession::AccountHttpSession(Poseidon::UniqueFile socket,
	boost::shared_ptr<const Poseidon::Http::AuthInfo> auth_info, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_auth_info(std::move(auth_info)), m_prefix(std::move(prefix))
{
}
AccountHttpSession::~AccountHttpSession(){
}

boost::shared_ptr<const ServletCallback> AccountHttpSession::create_servlet(const std::string &uri, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	if(!g_servlet_map.insert(std::make_pair(uri, servlet)).second){
		LOG_EMPERY_GATE_WESTWALK_ERROR("Duplicate account HTTP servlet: uri = ", uri);
		DEBUG_THROW(Exception, sslit("Duplicate account HTTP servlet"));
	}
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> AccountHttpSession::get_servlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_servlet_map.find(uri);
	if(it == g_servlet_map.end()){
		return { };
	}
	return it->second.lock();
}

boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> AccountHttpSession::predispatch_request(
	Poseidon::Http::RequestHeaders &request_headers, Poseidon::StreamBuffer &entity)
{
	check_and_throw_if_unauthorized(m_auth_info, get_remote_info(), request_headers);

	return Poseidon::Http::Session::predispatch_request(request_headers, entity);
}

void AccountHttpSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_GATE_WESTWALK(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted account HTTP request from ", get_remote_info());

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_GATE_WESTWALK_WARNING("Inacceptable account HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = get_servlet(uri);
	if(!servlet){
		LOG_EMPERY_GATE_WESTWALK_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	Poseidon::JsonObject result;
	try {
		result = (*servlet)(virtual_shared_from_this<AccountHttpSession>(), std::move(request_headers.get_params));
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(std::logic_error &e){
		LOG_EMPERY_GATE_WESTWALK_WARNING("std::logic_error thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_BAD_REQUEST);
	} catch(std::exception &e){
		LOG_EMPERY_GATE_WESTWALK_WARNING("std::exception thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_INTERNAL_SERVER_ERROR);
	}
	LOG_EMPERY_GATE_WESTWALK_DEBUG("Account response: uri = ", uri, ", result = ", result.dump());
	Poseidon::OptionalMap headers;
	headers.set(sslit("Access-Control-Allow-Origin"), "*");
	headers.set(sslit("Content-Type"), "application/json; charset=utf-8");
	send(Poseidon::Http::ST_OK, std::move(headers), Poseidon::StreamBuffer(result.dump()));
}

}
