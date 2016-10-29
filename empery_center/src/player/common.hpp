#ifndef EMPERY_CENTER_PLAYER_COMMON_HPP_
#define EMPERY_CENTER_PLAYER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <sstream>
#include "../singletons/account_map.hpp"
#include "../singletons/player_session_map.hpp"
#include "../account.hpp"
#include "../player_session.hpp"
#include "../msg/err_account.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"

/*
PLAYER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define PLAYER_SERVLET(MsgType_, account_arg_, session_arg_, req_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr< ::EmperyCenter::Account> &,	\
				const ::boost::shared_ptr< ::EmperyCenter::PlayerSession> &, MsgType_);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr< ::EmperyCenter::PlayerSession> &session_, ::Poseidon::StreamBuffer payload_)	\
			{	\
				PROFILE_ME;	\
				const auto account_uuid_ = ::EmperyCenter::PlayerSessionMap::get_account_uuid(session_);	\
				if(!account_uuid_){	\
					return ::EmperyCenter::CbppResponse(::EmperyCenter::Msg::ERR_NOT_LOGGED_IN);	\
				}	\
				const auto account_ = ::EmperyCenter::AccountMap::require(account_uuid_);	\
				MsgType_ msg_(payload_);	\
				LOG_EMPERY_CENTER_TRACE("Received request: account_uuid = ", account_uuid_,	\
					", remote_info = ", session_->get_remote_info(), ", msg = ", msg_);	\
				return TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (account_, session_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(::EmperyCenter::PlayerSession::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(PlayerServlet, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr< ::EmperyCenter::Account> & account_arg_ __attribute__((__unused__)),	\
		const ::boost::shared_ptr< ::EmperyCenter::PlayerSession> & session_arg_ __attribute__((__unused__)),	\
		MsgType_ req_arg_	\
		)

#define PLAYER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define PLAYER_THROW(code_)             PLAYER_THROW_MSG(code_, sslit(""))

namespace EmperyCenter {

using Response = CbppResponse;

}

#endif
