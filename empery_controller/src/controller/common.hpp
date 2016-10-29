#ifndef EMPERY_CONTROLLER_CONTROLLER_COMMON_HPP_
#define EMPERY_CONTROLLER_CONTROLLER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/job_base.hpp>
#include "../controller_session.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"
#include "../../../empery_center/src/msg/kill.hpp"
#include "../../../empery_center/src/msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"

/*
CONTROLLER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define CONTROLLER_SERVLET(MsgType_, session_arg_, req_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(ControllerServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr< ::EmperyController::ControllerSession> &, MsgType_);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(ControllerServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr< ::EmperyController::ControllerSession> &session_, ::Poseidon::StreamBuffer payload_)	\
			{	\
				PROFILE_ME;	\
				MsgType_ msg_(::std::move(payload_));	\
				LOG_EMPERY_CONTROLLER_TRACE("Received request from ", session_->get_remote_info(), ": ", msg_);	\
				return TOKEN_CAT3(ControllerServlet, __LINE__, Proc_) (session_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(ControllerSession::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(ControllerServlet, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(ControllerServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr< ::EmperyController::ControllerSession> & session_arg_ __attribute__((__unused__)),	\
		MsgType_ req_arg_	\
		)	\

#define CONTROLLER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define CONTROLLER_THROW(code_)             CONTROLLER_THROW_MSG(code_, sslit(""))

namespace EmperyController {

namespace Msg = ::EmperyCenter::Msg;

using Response = CbppResponse;

}

#endif
