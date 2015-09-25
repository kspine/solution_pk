#ifndef EMPERY_PROMOTION_ACCOUNT_COMMON_HPP_
#define EMPERY_PROMOTION_ACCOUNT_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/optional_map.hpp>
#include <poseidon/json.hpp>
#include "../account_http_session.hpp"
#include "../log.hpp"

/*
ACCOUNT_SERVLET(请求 URI, 会话形参名, GET 参数){
	// 处理逻辑
}
*/
#define ACCOUNT_SERVLET(uri_, sessionArg_, paramsArg_)	\
	namespace EmperyPromotion {	\
		namespace {	\
			namespace Impl_ {	\
				::Poseidon::JsonObject TOKEN_CAT3(AccountServlet, __LINE__, Proc_) (	\
					const ::boost::shared_ptr<AccountHttpSession> &, const ::Poseidon::OptionalMap &);	\
				void TOKEN_CAT3(AccountServlet, __LINE__, Entry_) (	\
					const ::boost::shared_ptr<AccountHttpSession> &session_, const ::Poseidon::OptionalMap &params_)	\
				{	\
					PROFILE_ME;	\
					::Poseidon::JsonObject result_ = TOKEN_CAT3(AccountServlet, __LINE__, Proc_) (session_, params_);	\
					LOG_EMPERY_PROMOTION_TRACE("Account servlet response: uri = ", uri_, ", result = ", result_.dump());	\
					::Poseidon::OptionalMap headers_;	\
					headers_.set("Content-Type", "application/json");	\
					headers_.set("Access-Control-Allow-Origin", "*");	\
					session_->send(::Poseidon::Http::ST_OK, ::std::move(headers_), ::Poseidon::StreamBuffer(result_.dump()));	\
				}	\
			}	\
		}	\
		MODULE_RAII(handles_){	\
			handles_.push(AccountHttpSession::createServlet(uri_, & Impl_:: TOKEN_CAT3(AccountServlet, __LINE__, Entry_)));	\
		}	\
	}	\
	::Poseidon::JsonObject EmperyPromotion::Impl_:: TOKEN_CAT3(AccountServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<AccountHttpSession> &sessionArg_, const ::Poseidon::OptionalMap &paramsArg_)	\

#define ACCOUNT_THROW(code_)				DEBUG_THROW(::Poseidon::Http::Exception, code_)

#endif
