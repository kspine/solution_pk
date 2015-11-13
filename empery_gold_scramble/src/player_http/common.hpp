#ifndef EMPERY_GOLD_SCRAMBLE_PLAYER_HTTP_COMMON_HPP_
#define EMPERY_GOLD_SCRAMBLE_PLAYER_HTTP_COMMON_HPP_

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
#include "../player_session.hpp"
#include "../log.hpp"

/*
PLAYER_HTTP_SERVLET(请求 URI, 会话形参名, GET 参数){
	// 处理逻辑
}
*/
#define PLAYER_HTTP_SERVLET(uri_, session_arg_, params_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::Poseidon::JsonObject TOKEN_CAT3(PlayerHttpServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr<PlayerSession> &, ::Poseidon::OptionalMap);	\
			::Poseidon::JsonObject TOKEN_CAT3(PlayerHttpServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr<PlayerSession> &session_, ::Poseidon::OptionalMap params_)	\
			{	\
				PROFILE_ME;	\
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Player HTTP servlet response: uri = ", uri_, ", params = { ",	\
					[&]{	\
						::std::ostringstream oss_;	\
						for(auto it_ = params_.begin(); it_ != params_.end(); ++it_)	\
							oss_ <<it_->first <<" = " <<it_->second <<"; ";	\
						return oss_.str();	\
					}(), " }");	\
				return TOKEN_CAT3(PlayerHttpServlet, __LINE__, Proc_) (session_, ::std::move(params_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(PlayerSession::create_http_servlet(uri_, & Impl_:: TOKEN_CAT3(PlayerHttpServlet, __LINE__, Entry_)));	\
	}	\
	::Poseidon::JsonObject Impl_:: TOKEN_CAT3(PlayerHttpServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<PlayerSession> &session_arg_ __attribute__((__unused__)),	\
		::Poseidon::OptionalMap params_arg_	\
		)

#define PLAYER_HTTP_THROW(code_)				DEBUG_THROW(::Poseidon::Http::Exception, code_)

#endif
