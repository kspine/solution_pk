#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/kill.hpp"
#include "../singletons/player_session_map.hpp"
#include "../singletons/activity_map.hpp"
#include "../player_session.hpp"
#include <poseidon/json.hpp>

namespace EmperyCenter{

ADMIN_SERVLET("activity/change", root, session, params){
	ActivityMap::reload();
	return Response();
}
//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/get_activity'
ADMIN_SERVLET("activity/get_activity", root, session, params){
	Poseidon::JsonArray activitys;
	ActivityMap::force_load_activitys(activitys);
	root[sslit("activitys")] = std::move(activitys);
	return Response();
}

//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update?activity_id=100001&activity_name=活动&begin_time=1234545&end_time=1234454'
ADMIN_SERVLET("activity/update", root, session, params){
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	const auto name_str        = params.get("activity_name");
	const auto begin_time      = boost::lexical_cast<std::uint64_t>(params.get("begin_time"));
	const auto end_time        = boost::lexical_cast<std::uint64_t>(params.get("end_time"));
	LOG_EMPERY_CENTER_FATAL("activity_id = ", activity_id, " activity_name = ",name_str, " begin_time = ",begin_time, " end_time_str = ", end_time);
	ActivityMap::force_update_activitys(activity_id,name_str,begin_time,end_time);
	return Response();
}

ADMIN_SERVLET("activity/delete", root, session, params){
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	ActivityMap::force_delete_activitys(activity_id);
	return Response();
}
//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/get_map_activity'
ADMIN_SERVLET("activity/get_map_activity", root, session, params){
	Poseidon::JsonArray activitys;
	ActivityMap::force_load_activitys_map(activitys);
	root[sslit("activitys_map")] = std::move(activitys);
	return Response();
}
//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update_map_activity?activity_id=100001&activity_name=活动&type=1&duration=20&target={"5000":{"2100034":5},"10000":{"2100034":10}}'
//url编码
//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update_map_activity?activity_id=100001&activity_name=活动&type=1&duration=20&target=%7b%225000%22%3a%7b%222100034%22%3a5%7d%2c%2210000%22%3a%7b%222100034%22%3a10%7d%7d'
ADMIN_SERVLET("activity/update_map_activity", root, session, params){
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	const auto name            = params.get("activity_name");
	const auto type            = boost::lexical_cast<std::uint32_t>(params.get("type"));
	const auto duration        = boost::lexical_cast<std::uint64_t>(params.get("duration"));
	const auto target          = params.get("target");
	ActivityMap::force_update_activitys_map(activity_id,name, type,duration,target);
	return Response();
}

ADMIN_SERVLET("activity/delete_map_activity", root, session, params){
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	ActivityMap::force_delete_activitys_map(activity_id);
	return Response();
}

//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/get_world_activity'
ADMIN_SERVLET("activity/get_world_activity", root, session, params){
	Poseidon::JsonArray activitys;
	ActivityMap::force_load_activitys_world(activitys);
	root[sslit("activitys_world")] = std::move(activitys);
	return Response();
}

//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update_world_activity?activity_id=100001&activity_name=活动&pro_activity_id=100000&own_activity_id=20000&type=1&
//  target={"0":500000}&drop={"guaibag10":1,"liangbag10":1,"mubag10":1,"shibag10":1}'

//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update_world_activity?activity_id=100001&activity_name=活动&pro_activity_id=100000&own_activity_id=20000&type=1&
//  target=%7b%220%22%3a500000%7d&drop=%7b%22guaibag10%22%3a1%2c%22liangbag10%22%3a1%2c%22mubag10%22%3a1%2c%22shibag10%22%3a1%7d'

ADMIN_SERVLET("activity/update_world_activity", root, session, params){
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	const auto name            = params.get("activity_name");
	const auto pro_activity_id = boost::lexical_cast<std::uint64_t>(params.get("pro_activity_id"));
	const auto own_activity_id = boost::lexical_cast<std::uint64_t>(params.get("own_activity_id"));
	const auto type            = boost::lexical_cast<std::uint32_t>(params.get("type"));
	const auto target          = params.get("target");
	const auto drop            = params.get("drop");
	ActivityMap::force_update_activitys_world(activity_id,name,pro_activity_id,own_activity_id,type,target,drop);
	return Response();
}

ADMIN_SERVLET("activity/delete_world_activity", root, session, params){
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	ActivityMap::force_delete_activitys_world(activity_id);
	return Response();
}

//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/get_rank_award'
ADMIN_SERVLET("activity/get_rank_award", root, session, params){
	Poseidon::JsonArray activitys_rank_award;
	ActivityMap::force_load_activitys_rank_award(activitys_rank_award);
	root[sslit("activitys_rank_award")] = std::move(activitys_rank_award);
	return Response();
}
//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update_rank_award?id=100001&activity_id=10000&rank_begin=1&rank_end=1&reward={"2100034":5}'
//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/update_rank_award?id=100001&activity_id=10000&rank_begin=1&rank_end=1&reward=%7b%222100034%22%3a5%7d'
ADMIN_SERVLET("activity/update_rank_award", root, session, params){
	const auto id  = boost::lexical_cast<std::uint64_t>(params.get("id"));
	const auto activity_id     = boost::lexical_cast<std::uint64_t>(params.get("activity_id"));
	const auto rank_begin      = boost::lexical_cast<std::uint32_t>(params.get("rank_begin"));
	const auto rank_end        = boost::lexical_cast<std::uint32_t>(params.get("rank_end"));
	const auto reward          = params.get("reward");
	ActivityMap::force_update_activitys_rank_award(id,activity_id,rank_begin,rank_end,reward);
	return Response();
}

//curl -vk 'http://admin:pass@192.168.1.203:13218/empery/admin/activity/delete_rank_award?id=100001'
ADMIN_SERVLET("activity/delete_rank_award", root, session, params){
	const auto id = boost::lexical_cast<std::uint64_t>(params.get("id"));
	ActivityMap::force_delete_activitys_rank_award(id);
	return Response();
}

}

