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
#include "data/map.hpp"
#include "data/map_object.hpp"
#include "data/global.hpp"
#include "../../empery_center/src/msg/sc_map.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/msg/err_castle.hpp"
#include "../../empery_center/src/cbpp_response.hpp"
#include "../../empery_center/src/attribute_ids.hpp"



namespace EmperyCluster {

namespace Msg {
	using namespace ::EmperyCenter::Msg;
}

using Response = ::EmperyCenter::CbppResponse;

AiControl::AiControl(boost::weak_ptr<MapObject> parent)
:m_parent_object(parent)
{

}

std::uint64_t AiControl::attack(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->attack(result,now);
}

void          AiControl::troops_attack(bool passive){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return;
	}
	parent_object->troops_attack(passive);
	return ;
}

std::uint64_t AiControl::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_attack(attacker,demage);
}

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id,
	AccountUuid owner_uuid, MapObjectUuid parent_object_uuid, bool garrisoned, boost::weak_ptr<ClusterClient> cluster,
	Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes)
	: m_map_object_uuid(map_object_uuid), m_map_object_type_id(map_object_type_id)
	, m_owner_uuid(owner_uuid), m_parent_object_uuid(parent_object_uuid),m_garrisoned(garrisoned), m_cluster(std::move(cluster))
	, m_coord(coord), m_attributes(std::move(attributes))
{
}
MapObject::~MapObject(){
}

std::uint64_t MapObject::pump_action(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto map_object_uuid    = get_map_object_uuid();
	const auto parent_object_uuid = get_parent_object_uuid();
	const auto garrisoned         = is_garrisoned();

	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		result = Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}

	const auto parent_map_object = WorldMap::get_map_object(parent_object_uuid);
	if(!parent_map_object && !is_monster()){
		result = Response(Msg::ERR_MAP_OBJECT_PARENT_GONE) << parent_object_uuid;
		return UINT64_MAX;
	}
	if(garrisoned){
	 	result = Response(Msg::ERR_MAP_OBJECT_IS_GARRISONED);
	 	return UINT64_MAX;
	}
	
	if(is_lost_attacked_target()){
		return search_attack();
	}
	
	
	//修正action
	if(!fix_attack_action()){
		return UINT64_MAX;
	}

	// 移动。
	if(!m_waypoints.empty()){
		return move(result);
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
		require_ai_control()->troops_attack();
		return require_ai_control()->attack(result,now);
	}
	ON_ACTION(ACT_DEPLOY_INTO_CASTLE){
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
		const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
		const auto cluster = get_cluster();
		if(!cluster){
			break;
		}
		Msg::KS_MapHarvestOverlay sreq;
		sreq.map_object_uuid = map_object_uuid.str();
		sreq.interval        = harvest_interval;
		auto sresult = cluster->send_and_wait(sreq);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			result = std::move(sresult);
			break;
		}
		return harvest_interval;
	}
	ON_ACTION(ACT_ENTER_CASTLE){
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
	}
	ON_ACTION(ACT_MONTER_REGRESS){
		return UINT64_MAX;
	}
	ON_ACTION(ACT_STAND_BY){
		const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
		return stand_by_interval;
	}
	ON_ACTION(ACT_HARVEST_STRATEGIC_RESOURCE){
		const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
		const auto cluster = get_cluster();
		if(!cluster){
			break;
		}
		Msg::KS_MapHarvestStrategicResource sreq;
		sreq.map_object_uuid = map_object_uuid.str();
		sreq.interval        = harvest_interval;
		auto sresult = cluster->send_and_wait(sreq);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_CLUSTER_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			result = std::move(sresult);
			break;
		}
		return harvest_interval;
	}
//=============================================================================
#undef ON_ACTION
		}
		break;
	default:
		LOG_EMPERY_CLUSTER_WARNING("Unknown action: action = ", static_cast<unsigned>(m_action));
		result = Response(Msg::ERR_UNKNOWN_MAP_OBJECT_ACTION) <<static_cast<unsigned>(m_action);
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

	const auto map_object_type_id = get_map_object_type_id();
	const auto map_object_type_data = Data::MapObjectType::require(map_object_type_id);

	std::uint64_t delay;
	const auto speed = map_object_type_data->speed * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0);
	if(speed <= 0){
		delay = UINT64_MAX;
	} else {
		delay = static_cast<std::uint64_t>(std::round(1000 / speed));
	}

	if(is_monster()){
		const unsigned monster_active_scope = Data::Global::as_unsigned(Data::Global::SLOT_MAP_MONSTER_ACTIVE_SCOPE);
		auto birth_x = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_X);
		auto birth_y = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_Y);
		const auto distance = get_distance_of_coords(new_coord, Coord(birth_x,birth_y));
		if(distance >= monster_active_scope){
			return search_attack();
		}
	}

	const auto new_cluster = WorldMap::get_cluster(new_coord);
	if(!new_cluster){
		LOG_EMPERY_CLUSTER_DEBUG("Lost connection to center server: new_coord = ", new_coord);
		result = Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_coord;
		return UINT64_MAX;
	}

	const auto retry_max_count = get_config<unsigned>("blocked_path_retry_max_count", 10);
	const auto wait_for_moving_objects = (m_blocked_retry_count < retry_max_count);
	result = get_move_result(new_coord, owner_uuid, wait_for_moving_objects);
	if(result.first == Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY){
		const auto retry_delay = get_config<std::uint64_t>("blocked_path_retry_delay", 500);
		++m_blocked_retry_count;
		return retry_delay;
	}
	m_blocked_retry_count = 0;

	if(result.first != Msg::ST_OK){
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
						Msg::SC_MapObjectStopped msg;
						msg.map_object_uuid = map_object_uuid.str();
						msg.action          = old_action;
						msg.param           = std::move(old_param);
						msg.error_code      = result.first;
						msg.error_message   = std::move(result.second);
						cluster->send_notification_by_account(get_owner_uuid(), msg);
					} catch(std::exception &e){
						LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
						cluster->shutdown(e.what());
					}
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

	set_coord(from_coord);

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

	notify_way_points(waypoints,action,action_param);

	m_waypoints    = std::move(waypoints);
	m_action       = action;
	m_action_param = std::move(action_param);
	reset_attack_target_own_uuid();
}

boost::shared_ptr<AiControl>  MapObject::require_ai_control(){
	PROFILE_ME;

	if(!m_ai_control){
		m_ai_control = boost::make_shared<AiControl>(virtual_weak_from_this<MapObject>());
	}
	return m_ai_control;
}

std::uint64_t MapObject::attack(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto target_object_uuid = MapObjectUuid(m_action_param);
	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		result = Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}
	const auto target_object = WorldMap::get_map_object(target_object_uuid);
	if(!target_object){
		result = Response(Msg::ERR_ATTACK_TARGET_LOST) << target_object_uuid;
		return UINT64_MAX;
	}
	const auto cluster = get_cluster();
	if(!cluster){
		result = Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<get_coord();
		return UINT64_MAX;
	}
	const auto emempy_type_data = Data::MapObjectType::get(target_object->get_map_object_type_id());
	if(!emempy_type_data){
		result = Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << target_object->get_map_object_type_id();
		return UINT64_MAX;
	}
	if((m_action != ACT_ATTACK) || (!m_waypoints.empty())){
		return UINT64_MAX;
	}
	if(!is_in_attack_scope(target_object_uuid)){
		return UINT64_MAX;
	}

	bool bDodge = false;
	bool bCritical = false;
	int result_type = IMPACT_NORMAL;
	std::uint64_t damage = 0;
	double addition_params = 1.0;//加成参数
	double damage_reduce_rate = 0.0;//伤害减免率
	double attack_rate = map_object_type_data->attack_speed;
	double doge_rate = emempy_type_data->doge_rate;
	double critical_rate = map_object_type_data->critical_rate;
	double critical_demage_plus_rate = map_object_type_data->critical_damage_plus_rate;
	double total_attack  = map_object_type_data->attack * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0);
	double total_defense = map_object_type_data->defence * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BONUS) / 1000.0);
	double relative_rate = Data::MapObjectRelative::get_relative(map_object_type_data->category_id,emempy_type_data->category_id);
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	auto ememy_solider_count = target_object->get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	
	if(attack_rate < 0.0001 && attack_rate > -0.0001){
		return UINT64_MAX;
	}
	//计算闪避，闪避成功，
	bDodge = Poseidon::rand32()%100 < doge_rate*100;

	if(bDodge){
		result_type = IMPACT_MISS;
	}else{
		//伤害计算
		if(target_object->get_map_object_type_id() != EmperyCenter::MapObjectTypeIds::ID_CASTLE ){
			damage = (1.0 +(soldier_count/10000.0) + (soldier_count - ememy_solider_count)/10000.0 * 0.5)*relative_rate*
			pow((total_attack*addition_params),2)/(total_attack*addition_params + total_defense*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate);
		}else{
			damage =  (1.0 +(soldier_count/10000.0))*relative_rate*
			pow((total_attack*addition_params),2)/(total_attack*addition_params + total_defense*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate);
		}
		result_type = IMPACT_NORMAL;
		damage = damage;
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
	cluster->send(msg);


	//判断受攻击者是否死亡
	if(!target_object->is_die()){
		target_object->require_ai_control()->on_attack(virtual_shared_from_this<MapObject>(),damage);
	}
	std::uint64_t attack_delay = static_cast<std::uint64_t>(1000.0 / attack_rate);
	return attack_delay;
}

std::uint64_t MapObject::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t damage){
	PROFILE_ME;

	if(!attacker){
		return UINT64_MAX;
	}
	//如果没有在攻击，则判断攻击者是否在自己的攻击范围之内，是则执行攻击，否则小范围内寻路攻击
	if(m_action != ACT_ATTACK && m_waypoints.empty() ){
		attack_new_target(attacker);
		troops_attack(true);
	}
	return UINT64_MAX;
}


bool MapObject::is_die(){
	PROFILE_ME;

	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	return (soldier_count > 0 ) ? false:true;
}

bool MapObject::is_in_attack_scope(MapObjectUuid target_object_uuid){
	PROFILE_ME;

	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		return false;
	}
	const auto shoot_range = get_shoot_range();
	const auto target_object = WorldMap::get_map_object(target_object_uuid);
	if(!target_object){
		return false;
	}
	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_object->get_coord());
	//攻击范围之内的军队
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

	return get_shoot_range() + 1;
}

void MapObject::troops_attack(bool passive){
	PROFILE_ME;
	
	if(is_monster()){
		return;
	}

	std::vector<boost::shared_ptr<MapObject>> friendly_map_objects;
	WorldMap::get_map_objects_by_account(friendly_map_objects,get_owner_uuid());
	if(friendly_map_objects.empty()){
		return;
	}
	for(auto it = friendly_map_objects.begin(); it != friendly_map_objects.end(); ++it){
		auto map_object = *it;
		if(!map_object || !map_object->is_idle() || !is_in_group_view_scope(map_object)){
			continue;
		}
		const auto enemy_object = WorldMap::get_map_object(MapObjectUuid(m_action_param));
		if(!enemy_object){
			return;
		}
		boost::shared_ptr<MapObject> near_enemy_object;
		if(passive&&map_object->get_new_enemy(enemy_object->get_owner_uuid(),near_enemy_object)){
			map_object->attack_new_target(near_enemy_object);
		}else{
			map_object->attack_new_target(enemy_object);
		}
	}
}

void   MapObject::notify_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,MapObject::Action &action, std::string &action_param){
	PROFILE_ME;

	const auto cluster = get_cluster();
	if(cluster){
		try {
			Msg::SC_MapWaypointsSet msg;
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
			cluster->send_notification_by_account(get_owner_uuid(), msg);
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
			cluster->shutdown(e.what());
		}
	}
}

bool    MapObject::fix_attack_action(){
	PROFILE_ME;

	if(m_action != ACT_ATTACK){
		return true;
	}

	//在攻击范围之内，直接进行攻击
	bool in_attack_scope = is_in_attack_scope(MapObjectUuid(m_action_param));
	if(in_attack_scope){
		m_waypoints.clear();
		notify_way_points(m_waypoints,m_action,m_action_param);
	}
	
	const auto target_object = WorldMap::get_map_object(MapObjectUuid(m_action_param));
	if(!target_object){
		return false;
	}

	if(!target_object->attacked_able()){
		return false;
	}

	if(!in_attack_scope&&m_waypoints.empty()){
		if(find_way_points(m_waypoints,get_coord(),target_object->get_coord())){
			notify_way_points(m_waypoints,m_action,m_action_param);
		}else{
			return false;
		}
	}
	return true;
}

bool    MapObject::find_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,Coord from_coord,Coord target_coord,bool precise){
	PROFILE_ME;

	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		return false;
	}

	std::vector<std::pair<signed char, signed char>> path;
	std::uint64_t distance_close_enough = 0;
	if(!precise){
		distance_close_enough = get_shoot_range();
	}
	if(find_path(path,from_coord, target_coord,get_owner_uuid(), 20, distance_close_enough)){
		for(auto it = path.begin(); it != path.end(); ++it){
			waypoints.emplace_back(it->first, it->second);
		}
		return true;
	}
	return false;
}

bool    MapObject::get_new_enemy(AccountUuid owner_uuid,boost::shared_ptr<MapObject> &new_enemy_map_object){
	PROFILE_ME;

	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_surrounding(map_objects,get_coord(),get_view_range());
	auto min_distance = UINT64_MAX;
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;

		if(map_object->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		if(!map_object->attacked_able()){
			continue;
		}
		if(map_object->get_owner_uuid() == owner_uuid){
			auto distance = get_distance_of_coords(get_coord(),map_object->get_coord());
			if(distance < min_distance){
				new_enemy_map_object = map_object;
				min_distance = distance;
			}
		}
	}
	if(min_distance != UINT64_MAX){
		return true;
	}
	return false;
}

void  MapObject::attack_new_target(boost::shared_ptr<MapObject> enemy_map_object){
	PROFILE_ME;

	if(!enemy_map_object)
		return;
	if(is_in_attack_scope(enemy_map_object->get_map_object_uuid())){
			set_action(get_coord(), m_waypoints, static_cast<MapObject::Action>(ACT_ATTACK),enemy_map_object->get_map_object_uuid().str());
		}else{
			std::deque<std::pair<signed char, signed char>> waypoints;
			if(find_way_points(waypoints,get_coord(),enemy_map_object->get_coord())){
				set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_ATTACK),enemy_map_object->get_map_object_uuid().str());
			}else{
				set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_STAND_BY),"");
			}
		}
}

std::uint64_t   MapObject::lost_target(){
	PROFILE_ME;

	if(is_monster()){
		monster_regress();
	}else{
		m_waypoints.clear();
		m_action = ACT_GUARD;
		m_action_param.clear();
		notify_way_points(m_waypoints,m_action,m_action_param);
	}
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
		set_action(get_coord(), waypoints, static_cast<MapObject::Action>(ACT_STAND_BY),"");
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

bool  MapObject::attacked_able(){
	PROFILE_ME;
	if(is_garrisoned()){
		return false;
	}
	if(is_monster() && (m_action == ACT_MONTER_REGRESS)){
		return false;
	}
	return true;
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
	if(!target_object){
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
		lost_target();
	}
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

std::uint64_t MapObject::get_shoot_range(){
	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		return 0;
	}
	const auto shoot_range = map_object_type_data->shoot_range;
	return shoot_range;
}




}
