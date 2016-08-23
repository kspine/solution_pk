#ifndef EMPERY_PROMOTION_SYNUSER_COMMON_HPP_
#define EMPERY_PROMOTION_SYNUSER_COMMON_HPP_

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
#include "../synuser_http_session.hpp"
#include "../log.hpp"

/*
SYNUSER_SERVLET(请求 URI, 会话形参名, GET 参数){
	// 处理逻辑
}
*/
#define SYNUSER_SERVLET(uri_, session_arg_, params_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::Poseidon::JsonObject TOKEN_CAT3(SynuserServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr<SynuserHttpSession> &, ::Poseidon::OptionalMap);	\
			::Poseidon::JsonObject TOKEN_CAT3(SynuserServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr<SynuserHttpSession> &session_, ::Poseidon::OptionalMap params_)	\
			{	\
				PROFILE_ME;	\
				return TOKEN_CAT3(SynuserServlet, __LINE__, Proc_) (session_, ::std::move(params_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(SynuserHttpSession::create_servlet(uri_, & Impl_:: TOKEN_CAT3(SynuserServlet, __LINE__, Entry_)));	\
	}	\
	::Poseidon::JsonObject Impl_:: TOKEN_CAT3(SynuserServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<SynuserHttpSession> &session_arg_, ::Poseidon::OptionalMap params_arg_)	\

#define SYNUSER_THROW(code_)				DEBUG_THROW(::Poseidon::Http::Exception, code_)

#endif
