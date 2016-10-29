#ifndef EMPERY_CENTER_LOG_LOG_COMMON_HPP_
#define EMPERY_CENTER_LOG_LOG_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/job_base.hpp>
#include "../log_session.hpp"
#include "../log.hpp"
#include "../../../empery_center/src/msg/kill.hpp"

/*
LOG_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define LOG_SERVLET(MsgType_, session_arg_, req_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			void TOKEN_CAT3(LogServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr< ::EmperyCenterLog::LogSession> &, MsgType_);	\
			void TOKEN_CAT3(LogServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr< ::EmperyCenterLog::LogSession> &session_, ::Poseidon::StreamBuffer payload_)	\
			{	\
				PROFILE_ME;	\
				MsgType_ msg_(::std::move(payload_));	\
				LOG_EMPERY_CENTER_LOG_TRACE("Received request from ", session_->get_remote_info(), ": ", msg_);	\
				return TOKEN_CAT3(LogServlet, __LINE__, Proc_) (session_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(LogSession::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(LogServlet, __LINE__, Entry_)));	\
	}	\
	void Impl_:: TOKEN_CAT3(LogServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr< ::EmperyCenterLog::LogSession> & session_arg_ __attribute__((__unused__)),	\
		MsgType_ req_arg_	\
		)	\

#define LOG_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define LOG_THROW(code_)             LOG_THROW_MSG(code_, sslit(""))

namespace EmperyCenterLog {

namespace Msg = ::EmperyCenter::Msg;

}

#endif
