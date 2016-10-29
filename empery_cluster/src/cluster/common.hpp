#ifndef EMPERY_CLUSTER_CLUSTER_COMMON_HPP_
#define EMPERY_CLUSTER_CLUSTER_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/job_base.hpp>
#include "../cluster_client.hpp"
#include "../log.hpp"
#include "../cbpp_response.hpp"
#include "../../../empery_center/src/msg/kill.hpp"

/*
CLUSTER_SERVLET(消息类型, 会话形参名, 消息形参名){
	// 处理逻辑
}
*/

#define CLUSTER_SERVLET(MsgType_, client_arg_, msg_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			::std::pair<long, ::std::string> TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (	\
				const ::boost::shared_ptr< ::EmperyCluster::ClusterClient> &, MsgType_);	\
			::std::pair<long, ::std::string> TOKEN_CAT3(ClusterServlet, __LINE__, Entry_) (	\
				const ::boost::shared_ptr< ::EmperyCluster::ClusterClient> &client_, ::Poseidon::StreamBuffer payload_)	\
			{	\
				PROFILE_ME;	\
				MsgType_ msg_(::std::move(payload_));	\
				LOG_EMPERY_CLUSTER_TRACE("Received message from ", client_->get_remote_info(), ": ", msg_);	\
				return TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (client_, ::std::move(msg_));	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(ClusterClient::create_servlet(MsgType_::ID, & Impl_:: TOKEN_CAT3(ClusterServlet, __LINE__, Entry_)));	\
	}	\
	::std::pair<long, ::std::string> Impl_:: TOKEN_CAT3(ClusterServlet, __LINE__, Proc_) (	\
		const ::boost::shared_ptr< ::EmperyCluster::ClusterClient> & client_arg_ __attribute__((__unused__)),	\
		MsgType_ msg_arg_	\
		)	\

#define CLUSTER_THROW_MSG(code_, msg_)   DEBUG_THROW(::Poseidon::Cbpp::Exception, code_, msg_)
#define CLUSTER_THROW(code_)             CLUSTER_THROW_MSG(code_, sslit(""))

namespace EmperyCluster {

namespace Msg = ::EmperyCenter::Msg;

using Response = CbppResponse;

}

#endif
