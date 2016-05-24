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
#include "../../empery_center/src/msg/sc_map.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/msg/err_castle.hpp"
#include "../../empery_center/src/attribute_ids.hpp"
#include "buff_ids.hpp"




namespace EmperyCluster {

namespace Msg {
	using namespace ::EmperyCenter::Msg;
}

namespace {
	void fill_buff_info(MapObject::BuffInfo &dest, const MapObject::BuffInfo &src){
		PROFILE_ME;

		dest.buff_id    = src.buff_id;
		dest.duration   = src.duration;
		dest.time_begin = src.time_begin;
		dest.time_end   = src.time_end;
	}
}

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

void          AiControl::troops_attack(boost::shared_ptr<MapObject> target,bool passive){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return;
	}
	parent_object->troops_attack(target,passive);
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

std::uint64_t AiControl::harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool force_attack){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->harvest_resource_crate(result,now,force_attack);
}

std::uint64_t AiControl::attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	PROFILE_ME;
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->attack_territory(result,now,forced_attack);
}

MapObject::MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id,
	AccountUuid owner_uuid, MapObjectUuid parent_object_uuid, bool garrisoned, boost::weak_ptr<ClusterClient> cluster,
	Coord coord, boost::container::flat_map<AttributeId, std::int64_t> attributes,boost::container::flat_map<BuffId, BuffInfo> buffs)
	: m_map_object_uuid(map_object_uuid), m_map_object_type_id(map_object_type_id)
	, m_owner_uuid(owner_uuid), m_parent_object_uuid(parent_object_uuid),m_garrisoned(garrisoned), m_cluster(std::move(cluster))
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
	if(!map_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}

	if(garrisoned){
	 	result = CbppResponse(Msg::ERR_MAP_OBJECT_IS_GARRISONED);
	 	return UINT64_MAX;
	}

	if(is_lost_attacked_target()){
		return search_attack();
	}

	//修正action
	if(!fix_attack_action(result)){
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
		require_ai_control()->troops_attack(target_object);
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
		return on_action_harvest_overplay(result,now);
	}
	ON_ACTION(ACT_HARVEST_OVERLAY_FORCE){
		return on_action_harvest_overplay(result,now,true);
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
		return on_action_harvest_strategic_resource(result,now);
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
		result = CbppResponse(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_coord;
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
		m_buffs.emplace(it->first, std::move(info));
	}
	it->second.duration = duration;
	it->second.time_begin = time_begin;
	it->second.time_end = saturated_add(time_begin, duration);
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
	m_waypoints    = std::move(waypoints);
	m_action       = action;
	m_action_param = std::move(action_param);
	notify_way_points(waypoints,action,m_action_param);
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
	double addition_params = 1.0;//加成参数
	double damage_reduce_rate = 0.0;//伤害减免率
	double attack_rate = map_object_type_data->attack_speed;
	double doge_rate = emempy_type_data->doge_rate;
	double critical_rate = map_object_type_data->critical_rate;
	double critical_demage_plus_rate = map_object_type_data->critical_damage_plus_rate;
	double total_attack  = map_object_type_data->attack * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0);
	double total_defense = emempy_type_data->defence * (1.0 + target_object->get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BONUS) / 1000.0);
	double relative_rate = Data::MapObjectRelative::get_relative(get_arm_relative_id(),target_object->get_arm_relative_id());
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
		if(!target_object->is_castle()){
			damage = (1.0 +(soldier_count/20000.0) + (soldier_count - ememy_solider_count)/20000.0 )*relative_rate*
			pow((total_attack*addition_params),2)/(total_attack*addition_params + total_defense*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate);
		}else{
			damage =  (1.0 +(soldier_count/20000.0))*relative_rate*
			pow((total_attack*addition_params),2)/(total_attack*addition_params + total_defense*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate);
		}
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
	if(m_action != ACT_ATTACK && m_waypoints.empty() && m_action != ACT_ENTER_CASTLE){
		attack_new_target(attacker);
	}
	troops_attack(attacker,true);
	return UINT64_MAX;
}

std::uint64_t MapObject::harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool force_attack){
		PROFILE_ME;
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
	if(m_action != ACT_HARVEST_RESOURCE_CRATE){
		return UINT64_MAX;
	}
	if(!is_in_attack_scope(target_resource_crate)){
		return UINT64_MAX;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(target_resource_crate->get_expiry_time() < utc_now){
		return UINT64_MAX;
	}

	std::uint64_t damage = 0;
	double addition_params = 1.0;//加成参数
	double damage_reduce_rate = 0.0;//伤害减免率
	double attack_rate = map_object_type_data->attack_speed;
	double total_attack  = map_object_type_data->attack;
	double total_defense = resource_crate_data->defence;
	double relative_rate = 1;
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);

	if(attack_rate < 0.0001 && attack_rate > -0.0001){
		return UINT64_MAX;
	}

	damage =  (1.0 +(soldier_count/20000.0))*relative_rate*
			pow((total_attack*addition_params),2)/(total_attack*addition_params + total_defense*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate);
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
	cluster->send(msg);

	std::uint64_t attack_delay = static_cast<std::uint64_t>(1000.0 / attack_rate);
	return attack_delay;
}

std::uint64_t MapObject::attack_territory(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	PROFILE_ME;

	if(m_action != ACT_ATTACK_TERRITORY){
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
	if(map_cell->is_in_protect()){
		result = CbppResponse(Msg::ERR_MAP_CELL_UNDER_PROTECTION);
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
	double addition_params = 1.0;//加成参数
	double damage_reduce_rate = 0.0;//伤害减免率
	double attack_rate = map_object_type_data->attack_speed;
	double total_attack  = map_object_type_data->attack;
	double total_defense = map_cell_ticket->defense;
	double relative_rate = 1;
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);

	if(attack_rate < 0.0001 && attack_rate > -0.0001){
		return UINT64_MAX;
	}

	damage =  (1.0 +(soldier_count/20000.0))*relative_rate*
			pow((total_attack*addition_params),2)/(total_attack*addition_params + total_defense*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate);
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
	cluster->send(msg);
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

	return get_shoot_range() + 1;
}

void MapObject::troops_attack(boost::shared_ptr<MapObject> target,bool passive){
	PROFILE_ME;

	if(is_monster()){
		return;
	}

	std::vector<boost::shared_ptr<MapObject>> friendly_map_objects;
	WorldMap::get_map_objects_by_account(friendly_map_objects,get_owner_uuid());
	if(friendly_map_objects.empty()){
		return;
	}
	if(!target){
			return;
	}
	for(auto it = friendly_map_objects.begin(); it != friendly_map_objects.end(); ++it){
		auto map_object = *it;
		if(!map_object || !map_object->is_idle() || !is_in_group_view_scope(map_object)){
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

bool    MapObject::fix_attack_action(std::pair<long, std::string> &result){
	PROFILE_ME;

	if( (m_action != ACT_ATTACK)
		&&(m_action != ACT_HARVEST_RESOURCE_CRATE)
		&&(m_action != ACT_ATTACK_TERRITORY)
	){
		return true;
	}
	if(is_die()){
		result = CbppResponse(Msg::ERR_ZERO_SOLDIER_COUNT);
		return false;
	}

	if(is_in_protect()){
		result = CbppResponse(Msg::ERR_SELF_UNDER_PROTECTION);
	    return false;
	 }
	 if(is_bunker()){
		 const auto garrisoning_battalion_type_id  = get_attribute(EmperyCenter::AttributeIds::ID_GARRISONING_BATTALION_TYPE_ID);
		if(!garrisoning_battalion_type_id){
			result = CbppResponse(Msg::ERR_MAP_OBJECT_IS_NOT_GARRISONED);
			return false;
		}
	 }
	Coord target_coord;
	bool in_attack_scope = false;
	if(m_action == ACT_ATTACK){
		const auto target_object = WorldMap::get_map_object(MapObjectUuid(m_action_param));
		if(!target_object){
			result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST);
			return false;
		}
		if(!target_object->attacked_able(result)){
			return false;
		}
		target_coord = target_object->get_coord();
		in_attack_scope = is_in_attack_scope(target_object);
	} else if( m_action == ACT_HARVEST_RESOURCE_CRATE ){
		const auto target_resource_crate = get_attack_resouce_crate();
		if(!target_resource_crate){
			result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST);
			return false;
		}
		target_coord = target_resource_crate->get_coord();
		in_attack_scope = is_in_attack_scope(target_resource_crate);
	} else if( m_action == ACT_ATTACK_TERRITORY){
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

	if(get_owner_uuid() == owner_uuid){
		return false;
	}
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_surrounding(map_objects,get_coord(),get_view_range());
	auto min_distance = UINT64_MAX;
	boost::shared_ptr<MapObject> select_object;
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;

		if(map_object->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		std::pair<long, std::string> reason;
		if(!map_object->attacked_able(reason)){
			continue;
		}
		if(map_object->is_monster()){
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
	if(is_in_attack_scope(enemy_map_object)){
			set_action(get_coord(), m_waypoints, static_cast<MapObject::Action>(ACT_ATTACK),enemy_map_object->get_map_object_uuid().str());
		}else{
			std::deque<std::pair<signed char, signed char>> waypoints;
			if(find_way_points(waypoints,get_coord(),enemy_map_object->get_coord(),false)){
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

bool  MapObject::attacked_able(std::pair<long, std::string> &reason){
	PROFILE_ME;
	if(is_die()){
		reason = CbppResponse(Msg::ERR_ZERO_SOLDIER_COUNT);
		return false;
	}
	if(is_garrisoned()){
		reason = CbppResponse(Msg::ERR_MAP_OBJECT_IS_GARRISONED);
		return false;
	}
	if(is_monster() && (m_action == ACT_MONTER_REGRESS)){
		reason = CbppResponse(Msg::ERR_TEMPORARILY_INVULNERABLE);
		return false;
	}
	if(is_defense_tower()){
		const auto defense_building_mission = get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BUILDING_MISSION);
		if(0 != defense_building_mission){
			reason = CbppResponse(Msg::ERR_DEFENSE_BUILDING_UPGRADE_IN_PROGESS);
			return false;
		}
	}
	if(is_in_protect()){
		reason = CbppResponse(Msg::ERR_BATTALION_UNDER_PROTECTION );
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

bool MapObject::is_in_protect(){
	PROFILE_ME;
	if(!is_castle()){
		return false;
	}
	if(is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION)&&!is_buff_in_effect(BuffIds::ID_CASTLE_PROTECTION_PREPARATION)){
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
		lost_target();
	}
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

std::uint64_t MapObject::get_shoot_range(){
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		return 0;
	}
	const auto shoot_range = map_object_type_data->shoot_range;
	return shoot_range;
}

MapObjectWeaponId MapObject::get_arm_relative_id(){
	//野怪
	const auto map_object_type_monster_data = Data::MapObjectTypeMonster::get(get_map_object_type_id());
	if(map_object_type_monster_data){
		return map_object_type_monster_data->arm_relative_id;
	}
	const auto map_object_type_data = get_map_object_type_data();
	if(!map_object_type_data){
		LOG_EMPERY_CLUSTER_DEBUG("No map object type data,id = ",get_map_object_type_id());
		return MapObjectWeaponId(0);
	}
	return map_object_type_data->category_id;
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
	if(is_castle() || is_bunker() || is_defense_tower()){
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
	if(m_action != ACT_ATTACK_TERRITORY){
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
	if(m_action != ACT_HARVEST_RESOURCE_CRATE){
		return { };
	}
	const auto target_resource_crate_uuid = ResourceCrateUuid(m_action_param);
	const auto target_resource_crate = WorldMap::get_resource_crate(target_resource_crate_uuid);
	return target_resource_crate;
}

std::uint64_t MapObject::on_action_harvest_overplay(std::pair<long, std::string> &result, std::uint64_t /*now*/,bool forced_attack){
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
}
std::uint64_t MapObject::on_action_harvest_strategic_resource(std::pair<long, std::string> &result, std::uint64_t /*now*/,bool forced_attack){
	const auto harvest_interval = get_config<std::uint64_t>("harvest_interval", 1000);
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
std::uint64_t MapObject::on_action_harvest_resource_crate(std::pair<long, std::string> &result, std::uint64_t now,bool forced_attack){
	const auto target_resource_crate_uuid = ResourceCrateUuid(m_action_param);
	const auto target_resource_crate = WorldMap::get_resource_crate(target_resource_crate_uuid);
	if(!target_resource_crate){
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
}
