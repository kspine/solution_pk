#ifndef EMPERY_CENTER_CLUSTER_COMMON_HPP_
#define EMPERY_CENTER_CLUSTER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/job_base.hpp>
#include "../cluster_session.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"

/*
CLUSTER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define CLUSTER_SERVLET(MsgType_, sessionArg_, reqArg_)	\
	namespace EmperyCenter {	\
		namespace {	\
			namespace Impl_ {	\
				ClusterSession::Result TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (	\
					const ::boost::shared_ptr<ClusterSession> &, MsgType_);	\
				ClusterSession::Result TOKEN_CAT3(ClusterServlet, __LINE__, Entry_) (	\
					const ::boost::shared_ptr<ClusterSession> &session_, ::Poseidon::StreamBuffer payload_)	\
				{	\
					PROFILE_ME;	\
					MsgType_ msg_(::std::move(payload_));	\
					LOG_EMPERY_CENTER_DEBUG("Received request from ", session_->getRemoteInfo(), ": ", msg_);	\
					return TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (session_, ::std::move(msg_));	\
				}	\
			}	\
		}	\
		MODULE_RAII(handles_){	\
			handles_.push(ClusterSession::createServlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(ClusterServlet, __LINE__, Entry_)));	\
		}	\
	}	\
	ClusterSession::Result EmperyCenter::Impl_:: TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<ClusterSession> & sessionArg_ __attribute__((__unused__)),	\
		MsgType_ reqArg_	\
		)	\

#define CLUSTER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define CLUSTER_THROW(code_)             CLUSTER_THROW_MSG(code_, sslit(""))

#endif
