#ifndef EMPERY_CONTROLLER_WORLD_COMMON_HPP_
#define EMPERY_CONTROLLER_WORLD_COMMON_HPP_

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
#include "../world_session.hpp"
#include "../log.hpp"
#include "../../../empery_center/src/cbpp_response.hpp"
#include "../../../empery_center/src/msg/kill.hpp"

/*
WORLD_SERVLET(请求 URI, 返回 JSON 形参名, 会话形参名, GET 参数){
	// 处理逻辑
	return Response(code) <<msg;
}
*/

#define WORLD_SERVLET(uri_, root_arg_, session_arg_, params_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(WorldServlet, __LINE__, Proc_) (	\
				::Poseidon::JsonObject &, const ::boost::shared_ptr< ::EmperyController::WorldSession> &, ::Poseidon::OptionalMap);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(WorldServlet, __LINE__, Entry_) (	\
				::Poseidon::JsonObject &root_, const ::boost::shared_ptr< ::EmperyController::WorldSession> &session_,	\
				::Poseidon::OptionalMap params_)	\
			{	\
				PROFILE_ME;	\
				LOG_EMPERY_CONTROLLER_DEBUG("World servlet request: uri = ", uri_);	\
				return TOKEN_CAT3(WorldServlet, __LINE__, Proc_) (root_, session_, ::std::move(params_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(WorldSession::create_servlet(uri_, & Impl_:: TOKEN_CAT3(WorldServlet, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(WorldServlet, __LINE__, Proc_) (	\
		::Poseidon::JsonObject & root_arg_ __attribute__((__unused__)),	\
		const ::boost::shared_ptr< ::EmperyController::WorldSession> & session_arg_ __attribute__((__unused__)),	\
		::Poseidon::OptionalMap params_arg_	\
		)

#define WORLD_THROW(code_)              DEBUG_THROW(::Poseidon::Http::Exception, (code_))

namespace EmperyController {

namespace Msg = ::EmperyCenter::Msg;

using Response = ::EmperyCenter::CbppResponse;

}

#endif
