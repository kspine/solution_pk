#ifndef EMPERY_CENTER_PLAYER_COMMON_HPP_
#define EMPERY_CENTER_PLAYER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/job_base.hpp>
#include <sstream>
#include "../singletons/account_map.hpp"
#include "../singletons/player_session_map.hpp"
#include "../player_session.hpp"
#include "../msg/cerr_account.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"

/*
PLAYER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define PLAYER_SERVLET_RAW(MsgType_, sessionArg_, reqArg_)	\
	namespace EmperyCenter {	\
		namespace {	\
			namespace Impl_ {	\
				::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServletRaw, __LINE__, Proc_) (	\
					const ::boost::shared_ptr<PlayerSession> &, MsgType_);	\
				::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServletRaw, __LINE__, Entry_) (	\
					const ::boost::shared_ptr<PlayerSession> &session_, ::Poseidon::StreamBuffer payload_)	\
				{	\
					PROFILE_ME;	\
					MsgType_ msg_(::std::move(payload_));	\
					LOG_EMPERY_CENTER_DEBUG("Received request from ", session_->getRemoteInfo(), ": ", msg_);	\
					return TOKEN_CAT3(PlayerServletRaw, __LINE__, Proc_) (session_, ::std::move(msg_));	\
				}	\
			}	\
		}	\
		MODULE_RAII(handles_){	\
			handles_.push(PlayerSession::createServlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(PlayerServletRaw, __LINE__, Entry_)));	\
		}	\
	}	\
	::std::pair<long, ::std::string> EmperyCenter::Impl_:: TOKEN_CAT3(PlayerServletRaw, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<PlayerSession> & sessionArg_ __attribute__((__unused__)),	\
		MsgType_ reqArg_	\
		)	\

#define PLAYER_SERVLET(MsgType_, accountUuidArg_, sessionArg_, reqArg_)	\
	namespace EmperyCenter {	\
		namespace {	\
			namespace Impl_ {	\
				::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (	\
					const ::EmperyCenter::AccountUuid &, const ::boost::shared_ptr<PlayerSession> &, MsgType_);	\
				::std::pair<long, ::std::string> TOKEN_CAT3(PlayerServlet, __LINE__, Entry_) (	\
					const ::boost::shared_ptr<PlayerSession> &session_, const ::Poseidon::StreamBuffer &payload_)	\
				{	\
					PROFILE_ME;	\
					const auto accountUuid_ = ::EmperyCenter::PlayerSessionMap::getAccountUuid(session_);	\
					if(!accountUuid_){	\
						return ::EmperyCenter::CbppResponse(::EmperyCenter::Msg::CERR_NOT_LOGGED_IN);	\
					}	\
					MsgType_ msg_(payload_);	\
					LOG_EMPERY_CENTER_DEBUG("Received request from account ", accountUuid_, " on ",	\
						session_->getRemoteInfo(), ": ", msg_);	\
					return TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (accountUuid_, session_, ::std::move(msg_));	\
				}	\
			}	\
		}	\
		MODULE_RAII(handles_){	\
			handles_.push(PlayerSession::createServlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(PlayerServlet, __LINE__, Entry_)));	\
		}	\
	}	\
	::std::pair<long, ::std::string> EmperyCenter::Impl_:: TOKEN_CAT3(PlayerServlet, __LINE__, Proc_) (	\
		const ::EmperyCenter::AccountUuid & accountUuidArg_ __attribute__((__unused__)),	\
		const ::boost::shared_ptr<PlayerSession> & sessionArg_ __attribute__((__unused__)),	\
		MsgType_ reqArg_	\
		)	\

#define PLAYER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define PLAYER_THROW(code_)             PLAYER_THROW_MSG(code_, sslit(""))

namespace EmperyCenter {

using Response = CbppResponse;

}

#endif
