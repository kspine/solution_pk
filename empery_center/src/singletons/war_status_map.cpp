#include "../precompiled.hpp"
#include "war_status_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>

namespace EmperyCenter {

using StatusInfo = WarStatusMap::StatusInfo;
/*
namespace {
	MULTI_INDEX_MAP(WarStatusContainer, StatusInfo,
		MULTI_MEMBER_INDEX(less_account_uuid)
		MULTI_MEMBER_INDEX(greater_account_uuid)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<WarStatusContainer> g_war_status_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("War status gc timer: now = ", now);

war_status_refresh_interval

const auto mail_box_map = g_mail_box_map.lock();
if(!mail_box_map){
return;
}

for(;;){
const auto it = mail_box_map->begin<1>();
if(it == mail_box_map->end<1>()){
break;
}
if(now < it->unload_time){
break;
}

// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
if((it->promise.use_count() <= 1) && it->mail_box && it->mail_box.unique()){
LOG_EMPERY_CENTER_DEBUG("Reclaiming mail box: account_uuid = ", it->account_uuid);
mail_box_map->erase<1>(it);
} else {
mail_box_map->set_key<1, 1>(it, now + 1000);
}
}
}
}
*/
}
