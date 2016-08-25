#include "precompiled.hpp"
#include "synuser_http_session.hpp"
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/json.hpp>

namespace EmperyPromotion {

using ServletCallback = SynuserHttpSession::ServletCallback;

namespace {
	std::map<std::string, boost::weak_ptr<const ServletCallback>> g_servlet_map;
}

SynuserHttpSession::SynuserHttpSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_prefix(std::move(prefix))
{
}
SynuserHttpSession::~SynuserHttpSession(){
}

boost::shared_ptr<const ServletCallback> SynuserHttpSession::create_servlet(const std::string &uri, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	if(!g_servlet_map.insert(std::make_pair(uri, servlet)).second){
		LOG_EMPERY_PROMOTION_ERROR("Duplicate synuser HTTP servlet: uri = ", uri);
		DEBUG_THROW(Exception, sslit("Duplicate synuser HTTP servlet"));
	}
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> SynuserHttpSession::get_servlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_servlet_map.find(uri);
	if(it == g_servlet_map.end()){
		return { };
	}
	return it->second.lock();
}

void SynuserHttpSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted synuser HTTP request from ", get_remote_info());

	Poseidon::OptionalMap headers;
	headers.set(sslit("Access-Control-Allow-Origin"),  "*");
	headers.set(sslit("Access-Control-Allow-Headers"), "Authorization");
	if(request_headers.verb == Poseidon::Http::V_OPTIONS){
		send(Poseidon::Http::ST_OK, std::move(headers));
		return;
	}

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_PROMOTION_WARNING("Inacceptable synuser HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(request_headers.verb != Poseidon::Http::V_POST){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = get_servlet(uri);
	if(!servlet){
		LOG_EMPERY_PROMOTION_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	Poseidon::JsonObject result;
	try {
		auto str = entity.dump();
		LOG_EMPERY_PROMOTION_DEBUG("Request entity: str = ", str);
		result = (*servlet)(virtual_shared_from_this<SynuserHttpSession>(), Poseidon::Http::optional_map_from_url_encoded(str));
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(std::logic_error &e){
		LOG_EMPERY_PROMOTION_WARNING("std::logic_error thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_BAD_REQUEST);
	} catch(std::exception &e){
		LOG_EMPERY_PROMOTION_WARNING("std::exception thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_INTERNAL_SERVER_ERROR);
	}
	LOG_EMPERY_PROMOTION_DEBUG("Synuser response: uri = ", uri, ", result = ", result.dump());
	headers.set(sslit("Content-Type"), "application/json; charset=utf-8");
	send(Poseidon::Http::ST_OK, std::move(headers), Poseidon::StreamBuffer(result.dump()));
}

}
