#include "precompiled.hpp"
#include "payment_session.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/json.hpp>

namespace EmperyCenter {

using ServletCallback = PaymentHttpSession::ServletCallback;

namespace {
	boost::container::flat_map<std::string, boost::weak_ptr<const ServletCallback>> g_servlet_map;
}

PaymentHttpSession::PaymentHttpSession(Poseidon::UniqueFile socket,
	boost::shared_ptr<const Poseidon::Http::AuthInfo> auth_info, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_auth_info(std::move(auth_info)), m_prefix(std::move(prefix))
{
}
PaymentHttpSession::~PaymentHttpSession(){
}

boost::shared_ptr<const ServletCallback> PaymentHttpSession::create_servlet(const std::string &uri, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[uri];
	if(!weak.expired()){
		LOG_EMPERY_CENTER_ERROR("Duplicate payment HTTP servlet: uri = ", uri);
		DEBUG_THROW(Exception, sslit("Duplicate payment HTTP servlet"));
	}
	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> PaymentHttpSession::get_servlet(const std::string &uri){
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

void PaymentHttpSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted payment HTTP request from ", get_remote_info());

	Poseidon::OptionalMap headers;
	headers.set(sslit("Access-Control-Allow-Origin"),  "*");
	headers.set(sslit("Access-Control-Allow-Headers"), "Authorization");
	if(request_headers.verb == Poseidon::Http::V_OPTIONS){
		send(Poseidon::Http::ST_OK, std::move(headers));
		return;
	}
	check_and_throw_if_unauthorized(m_auth_info, get_remote_info(), request_headers, false, std::move(headers));

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_CENTER_WARNING("Inacceptable payment HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = get_servlet(uri);
	if(!servlet){
		LOG_EMPERY_CENTER_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	std::pair<long, std::string> result;
	Poseidon::JsonObject root;
	try {
		result = (*servlet)(root, virtual_shared_from_this<PaymentHttpSession>(), std::move(request_headers.get_params));
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(Poseidon::Cbpp::Exception &e){
		LOG_EMPERY_CENTER_WARNING("Poseidon::Cbpp::Exception thrown: what = ", e.what());
		result.first = e.status_code();
		result.second = e.what();
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
		result.second = e.what();
	}
	root[sslit("err_code")] = result.first;
	root[sslit("err_msg")] = std::move(result.second);
	LOG_EMPERY_CENTER_DEBUG("Sending response: ", root.dump());
	headers.set(sslit("Content-Type"), "application/json");
	send(Poseidon::Http::ST_OK, std::move(headers), Poseidon::StreamBuffer(root.dump()));
}

}
