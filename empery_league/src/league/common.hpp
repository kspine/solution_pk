#ifndef EMPERY_LEAGUE_LEAGUE_COMMON_HPP_
#define EMPERY_LEAGUE_LEAGUE_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/job_base.hpp>
#include "../league_session.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"
#include "../../../empery_center/src/msg/kill.hpp"
/*
DUNGEON_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define LEAGUE_SERVLET(MsgType_, client_arg_, msg_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(LeagueServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr< ::EmperyLeague::LeagueSession> &, MsgType_);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(LeagueServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr< ::EmperyLeague::LeagueSession> &client_, ::Poseidon::StreamBuffer payload_)	\
			{	\
				PROFILE_ME;	\
				MsgType_ msg_(::std::move(payload_));	\
				LOG_EMPERY_LEAGUE_TRACE("Received message from ", client_->get_remote_info(), ": ", msg_);	\
				return TOKEN_CAT3(LeagueServlet, __LINE__, Proc_) (client_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(LeagueSession::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(LeagueServlet, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(LeagueServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr< ::EmperyLeague::LeagueSession> & client_arg_ __attribute__((__unused__)),	\
		MsgType_ msg_arg_	\
		)	\

#define LEAGUE_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define LEAGUE_THROW(code_)             LEAGUE_THROW_MSG(code_, sslit(""))

namespace EmperyLeague {

namespace Msg = ::EmperyCenter::Msg;

using Response = CbppResponse;

}

#endif
