#ifndef EMPERY_CLUSTER_CLUSTER_COMMON_HPP_
#define EMPERY_CLUSTER_CLUSTER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/job_base.hpp>
#include "../cluster_client.hpp"
#include "../log.hpp"
#include "../../../empery_center/src/cbpp_response.hpp"

/*
CLUSTER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define CLUSTER_SERVLET(MsgType_, clientArg_, reqArg_)	\
	namespace EmperyCluster {	\
		namespace {	\
			namespace Impl_ {	\
				ClusterClient::Result TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (	\
					const ::boost::shared_ptr<ClusterClient> &, MsgType_);	\
				ClusterClient::Result TOKEN_CAT3(ClusterServlet, __LINE__, Entry_) (	\
					const ::boost::shared_ptr<ClusterClient> &client_, ::Poseidon::StreamBuffer payload_)	\
				{	\
					PROFILE_ME;	\
					MsgType_ msg_(::std::move(payload_));	\
					LOG_EMPERY_CLUSTER_DEBUG("Received request from ", client_->getRemoteInfo(), ": ", msg_);	\
					return TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (client_, ::std::move(msg_));	\
				}	\
			}	\
		}	\
		MODULE_RAII(handles_){	\
			handles_.push(ClusterClient::createServlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(ClusterServlet, __LINE__, Entry_)));	\
		}	\
	}	\
	ClusterClient::Result EmperyCluster::Impl_:: TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr<ClusterClient> & (clientArg_),	\
		MsgType_ (reqArg_)	\
		)	\

#define CLUSTER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define CLUSTER_THROW(code_)             CLUSTER_THROW_MSG(code_, sslit(""))

namespace EmperyCluster {

using EmperyCenter::CbppResponse;

}

#endif
