#include "precompiled.hpp"
#include "map_object.hpp"
#include "mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/json.hpp>
#include "cluster_client.hpp"
#include "singletons/world_map.hpp"
#include "checked_arithmetic.hpp"
#include "map_utilities.hpp"
#include "map_cell.hpp"
#include "map_object_type_ids.hpp"
#include "resource_crate.hpp"
#include "data/map.hpp"
#include "data/map_object.hpp"
#include "data/global.hpp"
#include "data/resource_crate.hpp"
#include "cbpp_response.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/msg/err_castle.hpp"
#include "../../empery_center/src/attribute_ids.hpp"
#include "buff_ids.hpp"




namespace EmperyCluster {

namespace Msg = ::EmperyCenter::Msg;

namespace {
	void fill_buff_info(MapObject::BuffInfo &dest, const MapObject::BuffInfo &src){
		PROFILE_ME;

		dest.buff_id    = src.buff_id;
		dest.duration   = src.duration;
		dest.time_begin = src.time_begin;
		dest.time_end   = src.time_end;
	}
}

MapObject::MapObject(MapObjectUuid map_object_uuid, std::uint64_t stamp, MapObjectTypeId map_object_type_id,
	AccountUuid owner_uuid,  LegionUuid legion_uuid, MapObjectUuid parent_object_uuid, bool garrisoned, boost::weak_ptr<ClusterClient> cluster,
	Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes,boost::container::flat_map<BuffId, BuffInfo> buffs)
	: m_map_object_uuid(map_object_uuid), m_stamp(stamp), m_map_object_type_id(map_object_type_id)
	, m_owner_uuid(owner_uuid), m_legion_uuid(legion_uuid), m_parent_object_uuid(parent_object_uuid),m_garrisoned(garrisoned), m_cluster(std::move(cluster))
	, m_coord(coord), m_attributes(std::move(attributes)),m_buffs(std::move(buffs))
{
}
MapObject::~MapObject(){
}

std::uint64_t MapObject::pump_action(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto map_object_uuid    = get_map_object_uuid();
	const auto garrisoned         = is_garrisoned();

	const auto map_object_type_data = get_map_object_type_data();
//	if(!map_object_type_data && !is_legion_warehouse()){
	if(!map_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}
	/*
	if(abs(map_object_type_data->attack) < 0.000001){
		return UINT64_MAX;
	}
	*/
	if(garrisoned){
	 	result = CbppResponse(Msg::ERR_MAP_OBJECT_IS_GARRISONED);
	 	return UINT64_MAX;
	}

	if(is_lost_attacked_target()){
		LOG_EMPERY_CLUSTER_DEBUG("lost attacked target,and prepare search attack,current action:",m_action);
		return search_attack();
	}

	//修正action
	if(!fix_attack_action(result)){
		LOG_EMPERY_CLUSTER_DEBUG("fix attack action failed,action cancel,current action:",m_action);
		return UINT64_MAX;
	}

	// 移动。
	if(!m_waypoints.empty()){
		return require_ai_control()->move(result);
	}

	switch(m_action){
		{
#define ON_ACTION(action_)	\
		}	\
		break;	\
	case (action_): {
//=============================================================================
	ON_ACTION(ACT_GUARD){
		// 无事可做。
	}
	ON_ACTION(ACT_ATTACK){
		const auto target_object_uuid = MapObjectUuid(m_action_param);
		const auto target_object = WorldMap::get_map_object(target_object_uuid);
		if(!target_object){
			break;
		}
		// TODO 战斗。
		return require_ai_control()->attack(result,now);
	}
/*	ON_ACTION(ACT_DEPLOY_INTO_CASTLE){
		const auto cluster = get_cluster();
		if(!cluster){
			break;
		}
		Msg::KS_MapDeployImmigrants sreq;
		sreq.map_object_uuid = map_object_uuid.str();
		sreq.castle_name     = m_action_param;
		auto sresult = cluster->send_and_wait(sreq);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			result = std::move(sresult);
			break;
		}
	}
	ON_ACTION(ACT_HARVEST_OVERLAY){
		return on_action_harvest_overplay(result,now);
	}
	ON_ACTION(ACT_HARVEST_OVERLAY_FORCE){
		return on_action_harvest_overplay(result,now,true);
	}
*/	ON_ACTION(ACT_ENTER_CASTLE){
		const auto cluster = get_cluster();
		if(!cluster){
			break;
		}
		Msg::KS_MapEnterCastle sreq;
		sreq.map_object_uuid = map_object_uuid.str();
		sreq.castle_uuid     = m_action_param;
		auto sresult = cluster->send_and_wait(sreq);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			result = std::move(sresult);
			break;
		}
		LOG_EMPERY_CLUSTER_DEBUG("enter castle finish,cancle action",m_action);
	}
	ON_ACTION(ACT_MONTER_REGRESS){
		LOG_EMPERY_CLUSTER_DEBUG("monster regress finish,cancel action",m_action);
		return UINT64_MAX;
	}
	ON_ACTION(ACT_STAND_BY){
		const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
		return stand_by_interval;
	}
	ON_ACTION(ACT_HARVEST_STRATEGIC_RESOURCE){
		return on_action_harvest_strategic_resource(result,now);
	}
	ON_ACTION(ACT_HARVEST_LEGION_RESOURCE){
		return on_action_harvest_legion_resource(result,now);
	}
	ON_ACTION(ACT_HARVEST_STRATEGIC_RESOURCE_FORCE){
		return on_action_harvest_strategic_resource(result,now,true);
	}
	ON_ACTION(ACT_HARVEST_RESOURCE_CRATE){
		return on_action_harvest_resource_crate(result,now);
	}
	ON_ACTION(ACT_HARVEST_RESOURCE_CRATE_FORCE){
		return on_action_harvest_resource_crate(result,now,true);
	}
	ON_ACTION(ACT_ATTACK_TERRITORY){
		return on_action_attack_territory(result,now);
	}
	ON_ACTION(ACT_ATTACK_TERRITORY_FORCE){
		return on_action_attack_territory(result,now,true);
	}
//=============================================================================
#undef ON_ACTION
		}
		break;
	default:
		LOG_EMPERY_CLUSTER_WARNING("Unknown action: action = ", static_cast<unsigned>(m_action));
		result = CbppResponse(Msg::ERR_UNKNOWN_MAP_OBJECT_ACTION) <<static_cast<unsigned>(m_action);
		break;
	}
	return UINT64_MAX;
}

std::uint64_t MapObject::move(std::pair<long, std::string> &result){
	PROFILE_ME;
	// const auto map_object_uuid = get_map_object_uuid();
	const auto owner_uuid      = get_owner_uuid();
	const auto coord           = get_coord();

	const auto waypoint  = m_waypoints.front();
	const auto new_coord = Coord(coord.x() + waypoint.first, coord.y() + waypoint.second);

	const auto map_object_type_data = get_map_object_type_data();
	std::uint64_t delay;
	const auto speed = map_object_type_data->speed * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_SPEED_ADD) / 1000.0;
	if(speed <= 0){
		LOG_EMPERY_CLUSTER_FATAL("speed <= 0,",speed, " map_object_type_data->speed:",map_object_type_data->speed," get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0",get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0,
		" get_attribute(EmperyCenter::AttributeIds::ID_SPEED_ADD):",get_attribute(EmperyCenter::AttributeIds::ID_SPEED_ADD) / 1000.0);
		delay = UINT64_MAX;
	} else {
		delay = static_cast<std::uint64_t>(std::round(1000 / speed));
	}

	const auto new_cluster = WorldMap::get_cluster(new_coord);
	if(!new_cluster){
		LOG_EMPERY_CLUSTER_DEBUG("Lost connection to center server: new_coord = ", new_coord);
		result = CbppResponse(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_coord;
		return UINT64_MAX;
	}

	const auto retry_max_count = get_config<unsigned>("blocked_path_retry_max_count", 10);
	const auto wait_for_moving_objects = (m_blocked_retry_count < retry_max_count);
	result = get_move_result(new_coord, owner_uuid, wait_for_moving_objects);
	if(result.first == Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY){
		const auto retry_delay = get_config<std::uint64_t>("blocked_path_retry_delay", 500);
		if(m_action != ACT_ENTER_CASTLE){
			++m_blocked_retry_count;
		}
		return retry_delay;
	}
	m_blocked_retry_count = 0;

	if(result.first != Msg::ST_OK){
		// XXX 根据不同 action 计算路径。
		const auto to_coord = std::accumulate(m_waypoints.begin(), m_waypoints.end(), coord,
			[](Coord c, std::pair<signed char, signed char> d){ return Coord(c.x() + d.first, c.y() + d.second); });
		std::deque<std::pair<signed char, signed char>> new_waypoints;
		if(find_way_points(new_waypoints, coord, to_coord,true) || !new_waypoints.empty()){
			notify_way_points(new_waypoints, m_action, m_action_param);
			m_waypoints = std::move(new_waypoints);
			return 0;
		}
		return UINT64_MAX;
	}

	set_coord(new_coord);

	m_waypoints.pop_front();
	m_blocked_retry_count = 0;
	return delay;
}

Coord MapObject::get_coord() const {
	return m_coord;
}
void MapObject::set_coord(Coord coord){
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

std::int64_t MapObject::get_attribute(AttributeId map_object_attr_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(map_object_attr_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void MapObject::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}
void MapObject::set_attributes_no_synchronize(boost::container::flat_map<AttributeId, std::int64_t> modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		m_attributes.emplace(it->first, 0);
	}

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		auto &value = m_attributes.at(it->first);
		if(value == it->second){
			continue;
		}
		value = it->second;
	}
}

MapObject::BuffInfo MapObject::get_buff(BuffId buff_id) const{
	PROFILE_ME;

	BuffInfo info = { };
	info.buff_id = buff_id;
	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return info;
	}
	fill_buff_info(info, it->second);
	return info;
}

bool MapObject::is_buff_in_effect(BuffId buff_id) const {
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return false;
	}
	const auto time_end = it->second.time_end;
	if(time_end == 0){
		return false;
	}
	const auto utc_now = Poseidon::get_utc_time();
	return utc_now < time_end;
}
void MapObject::get_buffs(std::vector<BuffInfo> &ret) const{
		PROFILE_ME;

	ret.reserve(ret.size() + m_buffs.size());
	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		BuffInfo info = { };
		fill_buff_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void MapObject::set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration){
	auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		BuffInfo info = { };
		info.buff_id = buff_id;
		info.duration = duration;
		info.time_begin = time_begin;
		info.time_end = saturated_add(time_begin, duration);
		it = m_buffs.emplace(buff_id, std::move(info)).first;
	}
	auto &info = it->second;
	info.duration = duration;
	info.time_begin = time_begin;
	info.time_end = saturated_add(time_begin, duration);
}
void MapObject::clear_buff(BuffId buff_id) noexcept{
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	m_buffs.erase(it);
}

void MapObject::set_action(Coord from_coord, std::deque<std::pair<signed char, signed char>> waypoints, MapObject::Action action, std::string action_param){
	PROFILE_ME;

	const auto timer_proc = [this](const boost::weak_ptr<MapObject> &weak, std::uint64_t now){
		PROFILE_ME;

		const auto shared = weak.lock();
		if(!shared){
			return;
		}
		const auto map_object_uuid = get_map_object_uuid();
		LOG_EMPERY_CLUSTER_TRACE("Map object action timer: map_object_uuid = ", map_object_uuid);

		for(;;){
			if(now < m_next_action_time){
				if(m_action_timer){
					Poseidon::TimerDaemon::set_absolute_time(m_action_timer, m_next_action_time);
				}
				break;
			}

			std::uint64_t delay = UINT64_MAX;
			std::pair<long, std::string> result;
			try {
				delay = pump_action(result, now);
			} catch(std::exception &e){
				LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
				result.first = Msg::ERR_INVALID_ACTION_PARAM;
				try {
					result.second = e.what();
				} catch(std::exception &e){
					LOG_EMPERY_CLUSTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			if(delay == UINT64_MAX){
				auto old_action = m_action;
				auto old_param  = std::move(m_action_param);

				LOG_EMPERY_CLUSTER_DEBUG("Action stopped: map_object_uuid = ", map_object_uuid,
					", error_code = ", result.first, ", error_message = ", result.second);
				m_waypoints.clear();
				m_action = ACT_GUARD;
				m_action_param.clear();

				LOG_EMPERY_CLUSTER_DEBUG("Releasing action timer: map_object_uuid = ", map_object_uuid);
				m_action_timer.reset();

				const auto cluster = get_cluster();
				if(cluster){
					try {
						Msg::KS_MapObjectStopped msg;
						msg.map_object_uuid = map_object_uuid.str();
						msg.action          = old_action;
						msg.param           = std::move(old_param);
						msg.error_code      = result.first;
						msg.error_message   = std::move(result.second);
						cluster->send(msg);
					} catch(std::exception &e){
						LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
						cluster->shutdown(e.what());
					}
					WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
				}
				break;
			}
			m_next_action_time = checked_add(m_next_action_time, delay);
		}
	};

	m_waypoints.clear();
	m_blocked_retry_count = 0;
	m_action = ACT_GUARD;
	m_action_param.clear();


	const auto now = Poseidon::get_fast_mono_clock();

	if(!waypoints.empty() || (action != ACT_GUARD)){
		if(!m_action_timer){
			auto timer = Poseidon::TimerDaemon::register_absolute_timer(now, 200,
				std::bind(timer_proc, virtual_weak_from_this<MapObject>(), std::placeholders::_2));
			LOG_EMPERY_CLUSTER_DEBUG("Created action timer: map_object_uuid = ", get_map_object_uuid());
			m_action_timer = std::move(timer);
		}
		if(m_next_action_time < now){
			m_next_action_time = now;
		}
	}
	m_waypoints    = std::move(waypoints);
	m_action       = action;
	m_action_param = std::move(action_param);
	//set_coord(from_coord);
	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
	notify_way_points(m_waypoints,action,m_action_param);
	reset_attack_target_own_uuid();
}

boost::shared_ptr<AiControl>  MapObject::require_ai_control(){
	PROFILE_ME;

	if(!m_ai_control){
		const auto map_object_type_data = get_map_object_type_data();
		switch(map_object_type_data->ai_id){
			case AI_SOLIDER:
				m_ai_control = boost::make_shared<AiControl>(AI_SOLIDER,virtual_weak_from_this<MapObject>());
				break;
			case AI_MONSTER:
				m_ai_control = boost::make_shared<AiControlMonsterCommon>(AI_MONSTER,virtual_weak_from_this<MapObject>());
				break;
			case AI_BUILDING:
				m_ai_control = boost::make_shared<AiControlDefenseBuilding>(AI_BUILDING,virtual_weak_from_this<MapObject>());
				break;
			case AI_BUILDING_NO_ATTACK:
			    m_ai_control = boost::make_shared<AiControlDefenseBuildingNoAttack>(AI_BUILDING_NO_ATTACK,virtual_weak_from_this<MapObject>());
			    break;
			case AI_GOBLIN:
				m_ai_control = boost::make_shared<AiControlMonsterGoblin>(AI_GOBLIN,virtual_weak_from_this<MapObject>());
				break;
			default:
				LOG_EMPERY_CLUSTER_FATAL("invalid ai type:",map_object_type_data->ai_id);
				break;
		}
	}
	return m_ai_control;
}

std::uint64_t MapObject::attack(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;
	const auto target_object_uuid = MapObjectUuid(m_action_param);
	if(get_map_object_uuid() == target_object_uuid){
		result = CbppResponse(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS) << get_map_object_uuid();
		return UINT64_MAX;
	}
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}
	const auto target_object = WorldMap::get_map_object(target_object_uuid);
	if(!target_object){
		result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST) << target_object_uuid;
		return UINT64_MAX;
	}
	if(get_owner_uuid() == target_object->get_owner_uuid()){
		result = CbppResponse(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS) << get_owner_uuid();
		return UINT64_MAX;
	}
	if(!attacking_able(result)){
		return UINT64_MAX;
	}
	if(!target_object->attacked_able(result)){
		return UINT64_MAX;
	}
	result = is_under_protection(virtual_shared_from_this<MapObject>(),target_object);
	if(result.first != Msg::ST_OK){
		return UINT64_MAX;
	}
	if(is_level_limit(target_object)){
		result = CbppResponse(Msg::ERR_CANNOT_ATTACK_MONSTER_LEVEL_EXCEPT) << get_owner_uuid();
		return UINT64_MAX;
	}
	const auto cluster = get_cluster();
	if(!cluster){
		result = CbppResponse(Msg::ERR_CLUSTER_CONNECTION_LOST) <<get_coord();
		return UINT64_MAX;
	}
	const auto emempy_type_data = target_object->get_map_object_type_data();
	if(!emempy_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << target_object->get_map_object_type_id();
		return UINT64_MAX;
	}
	if((m_action != ACT_ATTACK) || (!m_waypoints.empty())){
		return UINT64_MAX;
	}
	if(!is_in_attack_scope(target_object)){
		return UINT64_MAX;
	}

	bool bDodge = false;
	bool bCritical = false;
	int result_type = IMPACT_NORMAL;
	std::uint64_t damage = 0;
	double k = 0.03;
	double attack_rate = map_object_type_data->attack_speed + get_attribute(EmperyCenter::AttributeIds::ID_RATE_OF_FIRE_ADD) / 1000.0;
	double doge_rate = emempy_type_data->doge_rate + get_attribute(EmperyCenter::AttributeIds::ID_DODGING_RATIO_ADD)/ 1000.0;
	double critical_rate = map_object_type_data->critical_rate + get_attribute(EmperyCenter::AttributeIds::ID_CRITICAL_DAMAGE_RATIO_ADD) / 1000.0;
	double critical_demage_plus_rate = map_object_type_data->critical_damage_plus_rate + get_attribute(EmperyCenter::AttributeIds::ID_CRITICAL_DAMAGE_MULTIPLIER_ADD) / 1000.0;
	double total_attack  = map_object_type_data->attack * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_ADD) / 1000.0;
	double total_defense = emempy_type_data->defence * (1.0 + target_object->get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BONUS) / 1000.0) + target_object->get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_ADD) / 1000.0;
	double relative_rate = Data::MapObjectRelative::get_relative(get_arm_attack_type(),target_object->get_arm_defence_type());
//	std::uint32_t hp =  map_object_type_data->hp ;
//	hp = (hp == 0 )? 1:hp;
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
//	if(soldier_count%hp != 0){
//		soldier_count = (soldier_count/hp + 1)*hp;
//	}
	//auto ememy_solider_count = target_object->get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	if(attack_rate < 0.0001 && attack_rate > -0.0001){
		return UINT64_MAX;
	}
	//计算闪避，闪避成功，
	bDodge = Poseidon::rand32()%100 < doge_rate*100;

	if(bDodge){
		result_type = IMPACT_MISS;
	}else{
		//伤害计算
		damage = total_attack * soldier_count * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
		result_type = IMPACT_NORMAL;
		damage = damage < 1 ? 1 : damage ;
		//暴击计算
		bCritical = Poseidon::rand32()%100 < critical_rate*100;
		if(bCritical){
			result_type = IMPACT_CRITICAL;
			damage = damage*(1.0+critical_demage_plus_rate);
		}
	}

	Msg::KS_MapObjectAttackAction msg;
	msg.attacking_account_uuid = get_owner_uuid().str();
	msg.attacking_object_uuid = get_map_object_uuid().str();
	msg.attacking_object_type_id = get_map_object_type_id().get();
	msg.attacking_coord_x = get_coord().x();
	msg.attacking_coord_y = get_coord().y();
	msg.attacked_account_uuid = target_object->get_owner_uuid().str();
	msg.attacked_object_uuid = target_object->get_map_object_uuid().str();
	msg.attacked_object_type_id = target_object->get_map_object_type_id().get();
	msg.attacked_coord_x = target_object->get_coord().x();
	msg.attacked_coord_y = target_object->get_coord().y();
	msg.result_type = result_type;
	msg.soldiers_damaged = damage;
	auto sresult = cluster->send_and_wait(msg);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	//判断受攻击者是否死亡
	if(!target_object->is_die()){
		target_object->require_ai_control()->on_attack(virtual_shared_from_this<MapObject>(),damage);
		require_ai_control()->troops_attack(target_object);
	}
	std::uint64_t attack_delay = static_cast<std::uint64_t>(1000.0 / attack_rate);
	return attack_delay;
}

std::uint64_t MapObject::on_attack_common(boost::shared_ptr<MapObject> attacker,std::uint64_t damage){
	PROFILE_ME;

	if(!attacker){
		return UINT64_MAX;
	}
	//如果没有在攻击，则判断攻击者是否在自己的攻击范围之内，是则执行攻击，否则小范围内寻路攻击
	if(m_action != ACT_ATTACK && m_waypoints.empty() && m_action != ACT_ENTER_CASTLE){
		attack_new_target(attacker);
	}
	require_ai_control()->troops_attack(attacker,true);
	return UINT64_MAX;
}

std::uint64_t MapObject::on_attack_goblin(boost::shared_ptr<MapObject> attacker,std::uint64_t damage){
	PROFILE_ME;

	if(!attacker){
		return UINT64_MAX;
	}
	const auto ai_id = require_ai_control()->get_ai_id();
	if(ai_id != AI_GOBLIN){
		return UINT64_MAX;
	}

	//移动逃跑
	if(m_waypoints.empty()){
		try{
			const auto ai_data = Data::MapObjectAi::get(ai_id);
			std::istringstream iss(ai_data->params);
			auto temp_array = Poseidon::JsonParser::parse_array(iss);
			auto random_left = static_cast<std::uint32_t>(temp_array.at(0).get<double>());
			auto random_right = static_cast<std::uint32_t>(temp_array.at(1).get<double>());
			const auto rand_x = Poseidon::rand32() % (saturated_sub(random_right, random_left) + 1) + random_left;
			const auto rand_y = Poseidon::rand32() % (saturated_sub(random_right, random_left) + 1) + random_left;
			auto direct_x = Poseidon::rand32() % 2 ? 1 : -1;
			auto direct_y = Poseidon::rand32() % 2 ? 1 : -1;
			std::int64_t  target_x = get_coord().x() + static_cast<std::int64_t>(rand_x)*direct_x;
			std::int64_t  target_y = get_coord().y() + static_cast<std::int64_t>(rand_y)*direct_y;
			const auto cluster_scope = WorldMap::get_cluster_scope(get_coord());
			auto left = cluster_scope.left();
			auto right = cluster_scope.left() + static_cast<std::int64_t>(cluster_scope.width());
			auto bottom = cluster_scope.bottom();
			auto top = cluster_scope.bottom() + static_cast<std::int64_t>(cluster_scope.height());
			target_x = ( left > target_x ? left  : target_x );
			target_x = ( right < target_x ? right : target_x );
			target_y = ( bottom > target_y ? bottom : target_y );
			target_y = ( target_y > top ? top : target_y);

		    std::deque<std::pair<signed char, signed char>> waypoints;
			if(find_way_points(waypoints,get_coord(),Coord(target_x,target_y),true) || !waypoints.empty()){
				set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_GUARD),"");
			}else{
				LOG_EMPERY_CLUSTER_DEBUG("goblin find way fail");
			}
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	return 0;
}

std::uint64_t MapObject::harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool force_attack){
	PROFILE_ME;

	const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}
	const auto target_resource_crate = get_attack_resouce_crate();
	if(!target_resource_crate){
		result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST) << m_action_param;
		return UINT64_MAX;
	}
	const auto cluster = get_cluster();
	if(!cluster){
		result = CbppResponse(Msg::ERR_CLUSTER_CONNECTION_LOST) <<get_coord();
		return UINT64_MAX;
	}
	const auto resource_crate_data = Data::ResourceCrate::get(target_resource_crate->get_resource_id(),target_resource_crate->get_amount_max());
	if(!resource_crate_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << target_resource_crate->get_resource_id();
		return UINT64_MAX;
	}
	if(m_action != ACT_HARVEST_RESOURCE_CRATE && m_action != ACT_HARVEST_RESOURCE_CRATE_FORCE){
		LOG_EMPERY_CLUSTER_DEBUG("harvest resouce create ,error action = ",m_action);
		return UINT64_MAX;
	}

	const auto utc_now = Poseidon::get_utc_time();
	if(target_resource_crate->get_expiry_time() < utc_now){
		LOG_EMPERY_CLUSTER_DEBUG("target resource crate expired , coord = ",target_resource_crate->get_coord());
		return UINT64_MAX;
	}

	double attack_rate = map_object_type_data->harvest_speed*(1 + get_attribute(EmperyCenter::AttributeIds::ID_HARVEST_SPEED_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_HARVEST_SPEED_ADD) / 1000.0;
	std::uint64_t damage = (std::uint64_t)std::max(harvest_interval / 60000.0 * get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT) * attack_rate*1000.0, 0.0);
	damage = damage < 1 ? 1 : damage ;
	const auto amount_remainging = target_resource_crate->get_amount_remaining();
	damage = (damage > amount_remainging) ? amount_remainging: damage;

	Msg::KS_MapHarvestResourceCrate msg;
	msg.attacking_account_uuid = get_owner_uuid().str();
	msg.attacking_object_uuid = get_map_object_uuid().str();
	msg.attacking_object_type_id = get_map_object_type_id().get();
	msg.attacking_coord_x = get_coord().x();
	msg.attacking_coord_y = get_coord().y();
	msg.resource_crate_uuid = target_resource_crate->get_resource_crate_uuid().str();
	msg.amount_harvested = damage;
	if(force_attack){
		msg.forced_attack = true;
	}
	msg.interval = harvest_interval;
	auto sresult = cluster->send_and_wait(msg);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}

	return harvest_interval;
}

std::uint64_t MapObject::attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	PROFILE_ME;

	if(m_action != ACT_ATTACK_TERRITORY && m_action != ACT_ATTACK_TERRITORY_FORCE){
		return UINT64_MAX;
	}

	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}
	const auto map_cell = get_attack_territory();
	if(!map_cell){
		result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST) << m_action_param;
		return UINT64_MAX;
	}
	bool occupied = map_cell->is_buff_in_effect(EmperyCenter::BuffIds::ID_OCCUPATION_MAP_CELL);
	if(((get_owner_uuid() == map_cell->get_owner_uuid())&&!occupied) || ((get_owner_uuid() == map_cell->get_occupier_owner_uuid())&&occupied)){
		result = CbppResponse(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS) << get_owner_uuid();
		return UINT64_MAX;
	}

	result = is_under_protection(virtual_shared_from_this<MapObject>(),map_cell);
	if(result.first != Msg::ST_OK){
		return UINT64_MAX;
	}
	const auto cluster = get_cluster();
	if(!cluster){
		result = CbppResponse(Msg::ERR_CLUSTER_CONNECTION_LOST) <<get_coord();
		return UINT64_MAX;
	}
	const auto map_cell_ticket = Data::MapCellTicket::get(map_cell->get_ticket_item_id());
	if(!map_cell_ticket){
		result = CbppResponse(Msg::ERR_LAND_PURCHASE_TICKET_NOT_FOUND) << map_cell->get_ticket_item_id();
		return UINT64_MAX;
	}
	if(!is_in_attack_scope(map_cell)){
		return UINT64_MAX;
	}

	std::uint64_t damage = 0;
	double k = 0.35;
	double attack_rate = map_object_type_data->attack_speed + get_attribute(EmperyCenter::AttributeIds::ID_RATE_OF_FIRE_ADD) / 1000.0;
	double total_attack  = map_object_type_data->attack * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_ADD) / 1000.0;;
	double total_defense = map_cell_ticket->defense;
	double relative_rate = Data::MapObjectRelative::get_relative(get_arm_attack_type(),map_cell_ticket->defence_type);
//	std::uint32_t hp =  map_object_type_data->hp ;
//	hp = (hp == 0 )? 1:hp;
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
//	if(soldier_count%hp != 0){
//		soldier_count = (soldier_count/hp + 1)*hp;
//	}

	if(attack_rate < 0.0001 && attack_rate > -0.0001){
		return UINT64_MAX;
	}

	damage = total_attack * soldier_count * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
	damage = damage < 1 ? 1 : damage ;

	Msg::KS_MapAttackMapCellAction msg;
	msg.attacking_account_uuid = get_owner_uuid().str();
	msg.attacking_object_uuid = get_map_object_uuid().str();
	msg.attacking_object_type_id = get_map_object_type_id().get();
	msg.attacking_coord_x = get_coord().x();
	msg.attacking_coord_y = get_coord().y();
	msg.attacked_account_uuid = map_cell->get_owner_uuid().str();
	msg.attacked_ticket_item_id = map_cell->get_ticket_item_id().get();
	msg.attacked_coord_x = map_cell->get_coord().x();
	msg.attacked_coord_y = map_cell->get_coord().y();
	msg.soldiers_damaged = damage;
	if(forced_attack){
		msg.forced_attack = true;
	}
	auto sresult = cluster->send_and_wait(msg);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	map_cell->on_attack(virtual_shared_from_this<MapObject>());
	std::uint64_t attack_delay = static_cast<std::uint64_t>(1000.0 / attack_rate);
	return attack_delay;
}

bool MapObject::is_die(){
	PROFILE_ME;

	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	return (soldier_count > 0 ) ? false:true;
}

bool MapObject::is_in_attack_scope(boost::shared_ptr<MapObject> target_object){
	PROFILE_ME;

	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return false;
	}
	const auto shoot_range = get_shoot_range();
	if(!target_object){
		return false;
	}

	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_object->get_coord());
	auto  min_distance = distance;
	std::set<std::uint64_t> distance_set;
	if(is_castle() && target_object->is_castle()){
		const auto distance_right_left = get_distance_of_coords(Coord(coord.x() + 1,coord.y()), target_object->get_coord());
		const auto distance_left_right = get_distance_of_coords(coord, Coord(target_object->get_coord().x()+1,target_object->get_coord().y()));
		const auto distance_right_right = get_distance_of_coords(Coord(coord.x() + 1,coord.y()),Coord(target_object->get_coord().x()+1,target_object->get_coord().y()));
		distance_set.insert(distance);
		distance_set.insert(distance_right_left);
		distance_set.insert(distance_left_right);
		distance_set.insert(distance_right_right);
		min_distance = *distance_set.begin() - 2;
	}else if(is_castle()){
		const auto distance_right_left = get_distance_of_coords(Coord(coord.x() + 1,coord.y()), target_object->get_coord());
		distance_set.insert(distance);
		distance_set.insert(distance_right_left);
		min_distance = *distance_set.begin() - 1;
	}else if(target_object->is_castle()){
		const auto distance_left_right = get_distance_of_coords(coord, Coord(target_object->get_coord().x()+1,target_object->get_coord().y()));
		distance_set.insert(distance);
		distance_set.insert(distance_left_right);
		min_distance = *distance_set.begin() - 1;
	}
	if(min_distance <= shoot_range){
		return true;
	}
	return false;
}

bool MapObject::is_in_attack_scope(boost::shared_ptr<ResourceCrate> target_resource_crate){
	PROFILE_ME;

	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return false;
	}
	const auto shoot_range = get_shoot_range();
	if(!target_resource_crate){
		return false;
	}

	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_resource_crate->get_coord());
	if(distance <= shoot_range){
		return true;
	}
	return false;
}

bool MapObject::is_in_attack_scope(boost::shared_ptr<MapCell> target_map_cell){
	PROFILE_ME;

	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return false;
	}
	const auto shoot_range = get_shoot_range();
	if(!target_map_cell){
		return false;
	}

	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_map_cell->get_coord());
	if(distance <= shoot_range){
		return true;
	}
	return false;
}

bool MapObject::is_in_group_view_scope(boost::shared_ptr<MapObject>& target_object){
	PROFILE_ME;

	if(!target_object){
		return false;
	}
	const auto view_range = get_view_range();

	const auto target_view_range = target_object->get_view_range();
	const auto troops_view_range = view_range > target_view_range ? view_range:target_view_range;
	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_object->get_coord());
	if(distance <= troops_view_range){
		return true;
	}

	return false;
}

std::uint64_t MapObject::get_view_range(){
	PROFILE_ME;
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return 0;
	}
	return map_object_type_data->view + get_attribute(EmperyCenter::AttributeIds::ID_SIGHT_RANGE_ADD) / 1000.0;
}

void MapObject::troops_attack(boost::shared_ptr<MapObject> target,bool passive){
	PROFILE_ME;

	if(is_monster()){
		return;
	}

	std::vector<boost::shared_ptr<MapObject>> friendly_map_objects;

	LOG_EMPERY_CLUSTER_ERROR("troops_attack legion_uuid = ", get_legion_uuid());
	if(!get_legion_uuid().str().empty())
	{
		WorldMap::get_map_objects_by_legion_uuid(friendly_map_objects,get_legion_uuid());
	}
	else
	{
		WorldMap::get_map_objects_by_account(friendly_map_objects,get_owner_uuid());
	}

	if(friendly_map_objects.empty()){
		return;
	}
	if(!target){
			return;
	}
	for(auto it = friendly_map_objects.begin(); it != friendly_map_objects.end(); ++it){
		auto map_object = *it;
		std::pair<long, std::string> reason;
		if(!map_object || !map_object->is_idle() || !is_in_group_view_scope(map_object) || !map_object->attacking_able(reason)){
			continue;
		}
		boost::shared_ptr<MapObject> near_enemy_object;
		if(passive&&map_object->get_new_enemy(target->get_owner_uuid(),near_enemy_object)){
			map_object->attack_new_target(near_enemy_object);
		}else{
			map_object->attack_new_target(target);
		}
	}
}

void   MapObject::notify_way_points(const std::deque<std::pair<signed char, signed char>> &waypoints,const MapObject::Action &action, const std::string &action_param){
	PROFILE_ME;

	const auto cluster = get_cluster();
	if(cluster){
		try {
			Msg::KS_MapWaypointsSet msg;
			msg.map_object_uuid = get_map_object_uuid().str();
			msg.x               = get_coord().x();
			msg.y               = get_coord().y();
			msg.waypoints.reserve(waypoints.size());
			for(auto it = waypoints.begin(); it != waypoints.end(); ++it){
				auto &waypoint = *msg.waypoints.emplace(msg.waypoints.end());
				waypoint.dx = it->first;
				waypoint.dy = it->second;
			}
			msg.action          = static_cast<unsigned>(action);
			msg.param           = action_param;
			if(m_action == ACT_ATTACK){
				const auto target_object_uuid = MapObjectUuid(m_action_param);
				const auto target_object = WorldMap::get_map_object(target_object_uuid);
				if(target_object){
					msg.target_x        = target_object->get_coord().x();
					msg.target_y        = target_object->get_coord().y();
				}
			} else if(m_action ==  ACT_HARVEST_RESOURCE_CRATE || m_action == ACT_HARVEST_RESOURCE_CRATE_FORCE){
				const auto target_resource_crate = get_attack_resouce_crate();
				if(target_resource_crate){
					msg.target_x       = target_resource_crate->get_coord().x();
					msg.target_y       = target_resource_crate->get_coord().y();
				}
			} else if(m_action == ACT_ATTACK_TERRITORY || m_action == ACT_ATTACK_TERRITORY_FORCE){
				const auto map_cell = get_attack_territory();
				if(map_cell){
					msg.target_x       = map_cell->get_coord().x();
					msg.target_y       = map_cell->get_coord().y();
				}
			}
			LOG_EMPERY_CLUSTER_DEBUG("KS_MapWaypointsSet = ",msg);			cluster->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
			cluster->shutdown(e.what());
		}
	}
}

bool    MapObject::fix_attack_action(std::pair<long, std::string> &result){
	PROFILE_ME;

	if( (m_action != ACT_ATTACK)
		&&(m_action != ACT_ATTACK_TERRITORY)
		&&(m_action != ACT_ATTACK_TERRITORY_FORCE)
	){
		return true;
	}
	if(!attacking_able(result)){
		return false;
	}
	Coord target_coord;
	bool in_attack_scope = false;
	if(m_action == ACT_ATTACK){
		const auto target_object = WorldMap::get_map_object(MapObjectUuid(m_action_param));
		if(!target_object){
			result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST);
			return false;
		}
		target_coord = target_object->get_coord();
		in_attack_scope = is_in_attack_scope(target_object);
	} else if( m_action == ACT_HARVEST_RESOURCE_CRATE || m_action == ACT_HARVEST_RESOURCE_CRATE_FORCE ){
		const auto target_resource_crate = get_attack_resouce_crate();
		if(!target_resource_crate){
			result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST);
			return false;
		}
		target_coord = target_resource_crate->get_coord();
		in_attack_scope = is_in_attack_scope(target_resource_crate);
	} else if( m_action == ACT_ATTACK_TERRITORY || m_action == ACT_ATTACK_TERRITORY_FORCE){
		const auto target_map_cell = get_attack_territory();
		if(!target_map_cell){
			result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST);
			return false;
		}
		target_coord = target_map_cell->get_coord();
		in_attack_scope = is_in_attack_scope(target_map_cell);
	}
	if(in_attack_scope&&!m_waypoints.empty()){
		m_waypoints.clear();
		notify_way_points(m_waypoints,m_action,m_action_param);
	}
	if(!in_attack_scope&&m_waypoints.empty()){
		if(find_way_points(m_waypoints,get_coord(),target_coord)){
			notify_way_points(m_waypoints,m_action,m_action_param);
		}else{
			result = CbppResponse(Msg::ERR_BROKEN_PATH);
			return false;
		}
	}
	return true;
}

bool    MapObject::find_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,Coord from_coord,Coord target_coord,bool precise){
	PROFILE_ME;

	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return false;
	}
	if(!move_able()){
		return false;
	}

	std::vector<std::pair<signed char, signed char>> path;
	std::uint64_t distance_close_enough = 0;
	if(!precise){
		distance_close_enough = get_shoot_range();
	}
	const auto distance_limit = get_config<unsigned>("path_recalculation_radius", 10);
	bool result = find_path(path,from_coord, target_coord,get_owner_uuid(), distance_limit, distance_close_enough);
	for(auto it = path.begin(); it != path.end(); ++it){
		waypoints.emplace_back(it->first, it->second);
	}
	return result;
}

bool    MapObject::get_new_enemy(AccountUuid owner_uuid,boost::shared_ptr<MapObject> &new_enemy_map_object){
	PROFILE_ME;

	if(get_owner_uuid() == owner_uuid){
		return false;
	}
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_surrounding(map_objects,get_coord(),get_view_range());
	auto min_distance = UINT64_MAX;
	boost::shared_ptr<MapObject> select_object;
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		const auto result = is_under_protection(virtual_shared_from_this<MapObject>(),map_object);
		if(result.first != Msg::ST_OK){
			continue;
		}
		if(is_level_limit(map_object)){
			continue;
		}
		if(is_castle() && map_object->is_castle()){
			continue;
		}
		std::pair<long, std::string> reason;
		if(!map_object->attacked_able(reason)){
			continue;
		}
		if(map_object->is_monster()){
			continue;
		}
		if(is_monster()&&map_object->is_building()){
			continue;
		}
		if(map_object->get_owner_uuid() == owner_uuid){
			auto distance = get_distance_of_coords(get_coord(),map_object->get_coord());
			if(distance < min_distance){
				min_distance = distance;
				select_object = map_object;
			}else if(distance == min_distance){
				if(select_object->get_attacked_prority() > map_object->get_attacked_prority()){
					select_object = map_object;
				}
			}
		}
	}
	if(min_distance != UINT64_MAX){
		new_enemy_map_object = select_object;
		return true;
	}
	return false;
}

void  MapObject::attack_new_target(boost::shared_ptr<MapObject> enemy_map_object){
	PROFILE_ME;

	if(!enemy_map_object)
		return;
	const auto result = is_under_protection(virtual_shared_from_this<MapObject>(),enemy_map_object);
	if(result.first != Msg::ST_OK){
		return;
	}
	if(is_level_limit(enemy_map_object)){
		LOG_EMPERY_CLUSTER_WARNING("attack target in level limit");
		return;
	}
	if(is_castle() && enemy_map_object->is_castle()){
		return;
	}
	std::pair<long, std::string> reason;
	if(!enemy_map_object->attacked_able(reason)){
		return;
	}

	if(is_monster()&&enemy_map_object->is_building()){
		return;
	}
	if(is_in_attack_scope(enemy_map_object)){
			set_action(get_coord(), m_waypoints, static_cast<MapObject::Action>(ACT_ATTACK),enemy_map_object->get_map_object_uuid().str());
		}else{
			std::deque<std::pair<signed char, signed char>> waypoints;
			if(find_way_points(waypoints,get_coord(),enemy_map_object->get_coord(),false)){
				set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_ATTACK),enemy_map_object->get_map_object_uuid().str());
			}else{
				set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_GUARD),"");
			}
	}
}

std::uint64_t   MapObject::lost_target_common(){
	PROFILE_ME;

	set_action(get_coord(), m_waypoints, static_cast<MapObject::Action>(ACT_GUARD),"");
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

std::uint64_t MapObject::lost_target_monster(){
	monster_regress();
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

void   MapObject::monster_regress(){
	PROFILE_ME;

	auto birth_x = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_X);
	auto birth_y = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_Y);
	std::deque<std::pair<signed char, signed char>> waypoints;
	if(find_way_points(waypoints,get_coord(),Coord(birth_x,birth_y),true)){
		set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_MONTER_REGRESS),"");
	}else{
		set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_GUARD),"");
	}

	const auto cluster = get_cluster();
	if(cluster){
		try {
			Msg::KS_MapHealMonster msg;
			msg.map_object_uuid = get_map_object_uuid().str();
			cluster->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
			cluster->shutdown(e.what());
		}
	}
}

bool  MapObject::is_monster(){
	PROFILE_ME;

	const auto map_object_type_monster_data = Data::MapObjectTypeMonster::get(get_map_object_type_id());
	if(!map_object_type_monster_data){
		return false;
	}
	return true;
}

bool  MapObject::attacked_able(std::pair<long, std::string> &reason){
	PROFILE_ME;

	if(is_garrisoned()){
		reason = CbppResponse(Msg::ERR_MAP_OBJECT_IS_GARRISONED);
		return false;
	}
	if(is_monster() && (m_action == ACT_MONTER_REGRESS)){
		reason = CbppResponse(Msg::ERR_TEMPORARILY_INVULNERABLE);
		return false;
	}
	return true;
}

bool   MapObject::attacking_able(std::pair<long, std::string> &reason){
	if(is_die()){
		reason = CbppResponse(Msg::ERR_ZERO_SOLDIER_COUNT);
		return false;
	}
	if(is_bunker()){
		 const auto garrisoning_battalion_type_id  = get_attribute(EmperyCenter::AttributeIds::ID_GARRISONING_BATTALION_TYPE_ID);
		if(!garrisoning_battalion_type_id){
			reason = CbppResponse(Msg::ERR_MAP_OBJECT_IS_NOT_GARRISONED);
			return false;
		}
	 }
	if(is_defense_tower()||is_bunker()){
		const auto defense_building_mission = get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BUILDING_MISSION);
		if(0 != defense_building_mission){
			reason = CbppResponse(Msg::ERR_DEFENSE_BUILDING_UPGRADE_IN_PROGESS);
			return false;
		}
	}
	 return true;
}

bool  MapObject::is_protectable(){
	if(is_building()){
		return true;
	}else{
		return false;
	}
}

bool MapObject::is_level_limit(boost::shared_ptr<MapObject> enemy_map_object){
	const auto map_object_type_data = enemy_map_object->get_map_object_type_data();
	if(map_object_type_data->map_object_chassis_id == MapObjectChassisId(2605000)){
		LOG_EMPERY_CLUSTER_WARNING("map_object_class_id = 2605000,level limit ignore");
		return false;
	}
	if(!is_monster() && enemy_map_object->is_monster()){
		const auto max_account_attack_level  = get_attribute(EmperyCenter::AttributeIds::ID_OWNER_MAX_ATTACK_MONSTER_LEVEL);
		const auto monster_level = enemy_map_object->get_monster_level();
		if(max_account_attack_level < monster_level){
			LOG_EMPERY_CLUSTER_DEBUG("account max attack level than monster level,max_account_attack_level = ",max_account_attack_level," monster_level = ",monster_level,
			" map_object_uuid = ",get_map_object_uuid(), " enemy_object_uuid = ",enemy_map_object->get_map_object_uuid());
			return true;
		}
	}
	return false;
}

std::int64_t MapObject::get_monster_level(){
	const auto map_object_type_monster_data = Data::MapObjectTypeMonster::get(get_map_object_type_id());
	if(!map_object_type_monster_data){
		return 0;
	}
	return map_object_type_monster_data->level;
}

bool  MapObject::is_lost_attacked_target(){
	PROFILE_ME;
	if(m_action != ACT_ATTACK){
		return false;
	}
	const auto target_object = WorldMap::get_map_object(MapObjectUuid(m_action_param));
	if(!target_object){
		return true;
	}

	if(target_object->is_garrisoned()){
		return true;
	}
	return false;
}

void  MapObject::reset_attack_target_own_uuid(){
	PROFILE_ME;
	if(m_action != ACT_ATTACK){
		m_target_own_uuid = {};
		return;
	}
	const auto target_object = WorldMap::get_map_object(MapObjectUuid(m_action_param));
	if(!target_object || target_object->is_monster()){
		m_target_own_uuid = {};
		return;
	}
	m_target_own_uuid = target_object->get_owner_uuid();
}

AccountUuid  MapObject::get_attack_target_own_uuid(){
	return m_target_own_uuid;
}

std::uint64_t MapObject::search_attack(){
	boost::shared_ptr<MapObject> near_enemy_object;
	bool is_get_new_enemy = get_new_enemy(get_attack_target_own_uuid(),near_enemy_object);
	if(is_get_new_enemy){
		attack_new_target(near_enemy_object);
	}else{
		require_ai_control()->on_lose_target();
	}
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

std::uint64_t MapObject::get_shoot_range(){
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return 0;
	}
	const auto shoot_range = map_object_type_data->shoot_range + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_RANGE_ADD) / 1000.0;
	return shoot_range;
}

unsigned MapObject::get_arm_attack_type(){
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		LOG_EMPERY_CLUSTER_DEBUG("No map object type data,id = ",get_map_object_type_id());
		return 0;
	}
	return map_object_type_data->attack_type;
}

unsigned MapObject::get_arm_defence_type(){
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		LOG_EMPERY_CLUSTER_DEBUG("No map object type data,id = ",get_map_object_type_id());
		return 0;
	}
	return map_object_type_data->defence_type;
}

int MapObject::get_attacked_prority(){
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return 0;
	}
	int prority = 0;
	switch(map_object_type_data->category_id.get()){
		case 1204001:
			prority = 1;
			break;
		case 1203002:
			prority = 2;
			break;
		case 1202002:
			prority = 2;
			break;
		case 1201002:
			prority = 4;
			break;
		case 1203001:
			prority = 5;
			break;
		case 1202001:
			prority = 6;
			break;
		case 1201001:
			prority = 7;
			break;
	}
	return prority;
}

bool  MapObject::is_building(){
	if(is_castle() || is_bunker() || is_defense_tower() || is_legion_warehouse()){
		return true;
	}
	return false;
}

bool  MapObject::is_castle(){
	const auto map_object_type_id = get_map_object_type_id();
	if( map_object_type_id == EmperyCenter::MapObjectTypeIds::ID_CASTLE){
		return true;
	}
	return false;
}

bool  MapObject::is_bunker(){
	const auto map_object_type_id = get_map_object_type_id();
	if( map_object_type_id == EmperyCenter::MapObjectTypeIds::ID_BATTLE_BUNKER){
		return true;
	}
	return false;
}

bool  MapObject::is_defense_tower(){
	const auto map_object_type_id = get_map_object_type_id();
	if( map_object_type_id == EmperyCenter::MapObjectTypeIds::ID_DEFENSE_TOWER){
		return true;
	}
	return false;
}

bool  MapObject::is_legion_warehouse(){
	const auto map_object_type_id = get_map_object_type_id();
	if( map_object_type_id == EmperyCenter::MapObjectTypeIds::ID_LEGION_WAREHOUSE){
		return true;
	}
	return false;
}

bool  MapObject::move_able(){
	if(is_building()){
		return false;
	}
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return false;
	}
	if(map_object_type_data->speed > 0){
		return true;
	}
	return false;
}

boost::shared_ptr<const Data::MapObjectType> MapObject::get_map_object_type_data(){
	if(!is_building()){
		return Data::MapObjectType::get(get_map_object_type_id());
	}
	std::uint32_t level = 1;
	if(is_building()){
		level = get_attribute(EmperyCenter::AttributeIds::ID_BUILDING_LEVEL);
	}
	const auto map_object_type_building_data = Data::MapObjectTypeBuilding::get(get_map_object_type_id(),level);
	if(!map_object_type_building_data){
		return { };
	}
	return Data::MapObjectType::get(map_object_type_building_data->arm_type_id);
}

boost::shared_ptr<MapCell> MapObject::get_attack_territory(){
	if(m_action != ACT_ATTACK_TERRITORY && m_action != ACT_ATTACK_TERRITORY_FORCE){
		return { };
	}
	std::istringstream iss(m_action_param);
	char ign;
	std::int64_t x,y;
	if(!(iss >> ign >> x >> ign >> y >> ign)){
		LOG_EMPERY_CLUSTER_DEBUG("Attack territory error param = ", m_action_param);
		return { };
	}
	const auto map_cell = WorldMap::get_map_cell(Coord(x,y));
	return map_cell;
}

boost::shared_ptr<ResourceCrate> MapObject::get_attack_resouce_crate(){
	if(m_action != ACT_HARVEST_RESOURCE_CRATE && m_action != ACT_HARVEST_RESOURCE_CRATE_FORCE){
		return { };
	}
	const auto target_resource_crate_uuid = ResourceCrateUuid(m_action_param);
	const auto target_resource_crate = WorldMap::get_resource_crate(target_resource_crate_uuid);
	return target_resource_crate;
}
/*
std::uint64_t MapObject::on_action_harvest_overplay(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
	const auto cluster = get_cluster();
	if(!cluster){
		return UINT64_MAX;
	}
	Msg::KS_MapHarvestOverlay sreq;
	sreq.map_object_uuid = get_map_object_uuid().str();
	sreq.interval        = harvest_interval;
	if(forced_attack){
		sreq.forced_attack   = true;
	}
	auto sresult = cluster->send_and_wait(sreq);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	return harvest_interval;
}*/
// unmerged代码
//std::uint64_t MapObject::on_action_harvest_strategic_resource(std::pair<long, std::string> &result, std::uint64_t /*now*/,bool forced_attack){
/*	const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
	const auto cluster = get_cluster();
	if(!cluster){
		return UINT64_MAX;
	}
	Msg::KS_MapHarvestStrategicResource sreq;
	sreq.map_object_uuid = get_map_object_uuid().str();
	sreq.interval        = harvest_interval;
	if(forced_attack){
		sreq.forced_attack   = true;
	}
	auto sresult = cluster->send_and_wait(sreq);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	return harvest_interval;
}
*/

// master代码
std::uint64_t MapObject::on_action_harvest_strategic_resource(std::pair<long, std::string> &result, std::uint64_t /*now*/,bool forced_attack){
	const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
	const auto cluster = get_cluster();
	if(!cluster){
		return UINT64_MAX;
	}
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return UINT64_MAX;
	}
	double attack_rate = map_object_type_data->harvest_speed*(1 + get_attribute(EmperyCenter::AttributeIds::ID_HARVEST_SPEED_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_HARVEST_SPEED_ADD) / 1000.0;
	Msg::KS_MapHarvestStrategicResource sreq;
	sreq.map_object_uuid = get_map_object_uuid().str();
	sreq.interval        = harvest_interval;
	sreq.amount_harvested = (std::uint64_t)std::max(harvest_interval / 60000.0 * get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT) * attack_rate*1000.0, 0.0);//防止被舍掉，center 中除以1000
	if(forced_attack){
		sreq.forced_attack   = true;
	}
	auto sresult = cluster->send_and_wait(sreq);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	return harvest_interval;
}

std::uint64_t MapObject::on_action_harvest_legion_resource(std::pair<long, std::string> &result, std::uint64_t /*now*/,bool forced_attack){
	const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
	const auto cluster = get_cluster();
	if(!cluster){
		return UINT64_MAX;
	}
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return UINT64_MAX;
	}
	double attack_rate = map_object_type_data->harvest_speed*(1 + get_attribute(EmperyCenter::AttributeIds::ID_HARVEST_SPEED_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_HARVEST_SPEED_ADD) / 1000.0;
	Msg::KS_MapHarvestLegionResource sreq;
	sreq.map_object_uuid = get_map_object_uuid().str();
	sreq.interval        = harvest_interval;
	sreq.amount_harvested = (std::uint64_t)std::max(harvest_interval / 60000.0 * get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT) * attack_rate, 0.0);
	if(forced_attack){
		sreq.forced_attack   = true;
	}
	sreq.param = m_action_param;
	auto sresult = cluster->send_and_wait(sreq);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	return harvest_interval;
}


std::uint64_t MapObject::on_action_harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	const auto target_resource_crate_uuid = ResourceCrateUuid(m_action_param);
	const auto target_resource_crate = WorldMap::get_resource_crate(target_resource_crate_uuid);
	if(!target_resource_crate){
		LOG_EMPERY_CLUSTER_FATAL("no target resouce crate,target_resource_crate_uuid",target_resource_crate_uuid);
		return UINT64_MAX;
	}
	return require_ai_control()->harvest_resource_crate(result,now,forced_attack);
}
std::uint64_t MapObject::on_action_attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	const auto map_cell = get_attack_territory();
	if(!map_cell){
		return UINT64_MAX;
	}
	return require_ai_control()->attack_territory(result,now,forced_attack);
}

bool       MapObject::is_in_monster_active_scope(){
	if(is_monster()){
		const auto coord           = get_coord();
		const auto waypoint  = m_waypoints.front();
		const auto new_coord = Coord(coord.x() + waypoint.first, coord.y() + waypoint.second);
		const unsigned monster_active_scope = Data::Global::as_unsigned(Data::Global::SLOT_MAP_MONSTER_ACTIVE_SCOPE);
		auto birth_x = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_X);
		auto birth_y = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_Y);
		const auto distance = get_distance_of_coords(new_coord, Coord(birth_x,birth_y));
		if(distance >= monster_active_scope){
			return false;
		}
	}
	return true;
}
}