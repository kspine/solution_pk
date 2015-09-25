#include "precompiled.hpp"
#include "chinapnr_http_session.hpp"
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/job_base.hpp>

namespace EmperyPromotion {

ChinaPnRHttpSession::ChinaPnRHttpSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_prefix(std::move(prefix))
{
}
ChinaPnRHttpSession::~ChinaPnRHttpSession(){
}

void ChinaPnRHttpSession::onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer & /* entity */){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted ChinaPnR HTTP request from ", getRemoteInfo());

	auto uri = Poseidon::Http::urlDecode(requestHeaders.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_PROMOTION_WARNING("Inacceptable ChinaPnR HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(requestHeaders.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = getServlet(uri);
	if(!servlet){
		LOG_EMPERY_PROMOTION_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	try {
		(*servlet)(virtualSharedFromThis<ChinaPnRHttpSession>(), requestHeaders.getParams);
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(std::logic_error &e){
		LOG_EMPERY_PROMOTION_WARNING("std::logic_error thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_BAD_REQUEST);
	} catch(std::exception &e){
		LOG_EMPERY_PROMOTION_WARNING("std::exception thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_INTERNAL_SERVER_ERROR);
	}
}

}
