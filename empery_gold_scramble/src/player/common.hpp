#ifndef EMPERY_GOLD_SCRAMBLE_PLAYER_COMMON_HPP_
#define EMPERY_GOLD_SCRAMBLE_PLAYER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/job_base.hpp>
#include <sstream>
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../msg/err_account.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"

/*
PLAYER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define PLAYER_SERVLET_RAW(MsgType_, session_arg_, req_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServletRaw, __LINE__, Proc_) (	\
				const ::boost::shared_ptr<PlayerSession> &, MsgType_);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServletRaw, __LINE__, Entry_) (	\
				const ::boost::shared_ptr<PlayerSession> &session_, ::Poseidon::StreamBuffer payload_)	\
			{	\
				PROFILE_ME;	\
				MsgType_ msg_(::std::move(payload_));	\
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Received request from ", session_->get_remote_info(), ": ", msg_);	\
				return TOKEN_CAT3(PlayerServletRaw, __LINE__, Proc_) (session_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(PlayerSession::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(PlayerServletRaw, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(PlayerServletRaw, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<PlayerSession> & session_arg_ __attribute__((__unused__)),	\
		MsgType_ req_arg_	\
		)	\

#define PLAYER_SERVLET(MsgType_, login_name_arg_, session_arg_, req_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (	\
				const ::std::string &, const ::boost::shared_ptr<PlayerSession> &, MsgType_);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr<PlayerSession> &session_, const ::Poseidon::StreamBuffer &payload_)	\
			{	\
				PROFILE_ME;	\
				const auto login_name_ = ::EmperyGoldScramble::PlayerSessionMap::get_login_name(session_);	\
				if(login_name_.empty()){	\
					return ::EmperyGoldScramble::CbppResponse(::EmperyGoldScramble::Msg::ERR_INVALID_SESSION_ID);	\
				}	\
				MsgType_ msg_(payload_);	\
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Received request from account ", login_name_, " on ",	\
					session_->get_remote_info(), ": ", msg_);	\
				return TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (login_name_, session_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(PlayerSession::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(PlayerServlet, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (	\
		const ::std::string & login_name_arg_ __attribute__((__unused__)),	\
		const ::boost::shared_ptr<PlayerSession> & session_arg_ __attribute__((__unused__)),	\
		MsgType_ req_arg_	\
		)	\

#define PLAYER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define PLAYER_THROW(code_)             PLAYER_THROW_MSG(code_, ::Poseidon::sslit(""))

namespace EmperyGoldScramble {

using Response = CbppResponse;

}

#endif
