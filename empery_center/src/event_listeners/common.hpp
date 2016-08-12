#ifndef EMPERY_CENTER_EVENT_LISTENERS_COMMON_HPP_
#define EMPERY_CENTER_EVENT_LISTENERS_COMMON_HPP_

#include <poseidon/cxx_ver.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/shared_nts.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/event_base.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include <sstream>
#include "../log.hpp"

/*
EVENT_LISTENER(事件类型, 事件形参名){
	// 处理逻辑
}
*/

#define EVENT_LISTENER(EventType_, event_arg_)	\
	namespace {	\
		namespace Impl_ {	\
			void TOKEN_CAT3(EventListener, __LINE__, Proc_) (	\
				const EventType_ &);	\
			void TOKEN_CAT3(EventListener, __LINE__, Entry_) (	\
				const ::boost::shared_ptr< EventType_ > &event_ptr_)	\
			{	\
				PROFILE_ME;	\
				return TOKEN_CAT3(EventListener, __LINE__, Proc_) (*event_ptr_);	\
			}	\
		}	\
	}	\
	MODULE_RAII(handles_){	\
		handles_.push(::Poseidon::EventDispatcher::register_listener< EventType_ >(	\
			& Impl_:: TOKEN_CAT3(EventListener, __LINE__, Entry_)));	\
	}	\
	void Impl_:: TOKEN_CAT3(EventListener, __LINE__, Proc_) (	\
		const EventType_ &event_arg_	\
		)

#endif
