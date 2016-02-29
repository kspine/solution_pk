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
#include "data/map.hpp"
#include "data/map_object.hpp"
#include "data/global.hpp"
#include "../../empery_center/src/msg/sc_map.hpp"
#include "../../empery_center/src/msg/ks_map.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/msg/err_castle.hpp"
#include "../../empery_center/src/map_object_type_ids.hpp"
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

std::uint64_t AiControl::move(std::pair<long, std::string> &result){
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->move(result);
}
std::uint64_t AiControl::attack(std::pair<long, std::string> &result, std::uint64_t now){
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->attack(result,now);
}
std::uint64_t AiControl::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t demage){
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_attack(attacker,demage);
}

std::uint64_t AiControl::on_die(boost::shared_ptr<MapObject> attacker){
	const auto parent_object = m_parent_object.lock();
	if(!parent_object){
		return UINT64_MAX;
	}
	return parent_object->on_die(attacker);
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
	
	const auto parent_map_object = WorldMap::get_map_object(parent_object_uuid);
	if(!parent_map_object){
		result = Response(Msg::ERR_MAP_OBJECT_PARENT_GONE) << parent_object_uuid;
		return UINT64_MAX;
	}
	if(garrisoned){
	 	result = Response(Msg::ERR_MAP_OBJECT_IS_GARRISONED);
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
	//const auto map_object_uuid = get_map_object_uuid();
	const auto owner_uuid      = get_owner_uuid();
	const auto coord           = get_coord();
	
	const auto waypoint  = m_waypoints.front();
	const auto new_coord = Coord(coord.x() + waypoint.dx, coord.y() + waypoint.dy);
	const auto delay     = waypoint.delay;
	// 检测阻挡。
	const auto new_map_cell = WorldMap::get_map_cell(new_coord);
	if(new_map_cell){
		const auto cell_owner_uuid = new_map_cell->get_owner_uuid();
		if(cell_owner_uuid && (owner_uuid != cell_owner_uuid)){
			LOG_EMPERY_CLUSTER_DEBUG("Blocked by a cell owned by another player's territory: cell_owner_uuid = ", cell_owner_uuid);
			result = Response(Msg::ERR_BLOCKED_BY_OTHER_TERRITORY) <<cell_owner_uuid;
			return UINT64_MAX;
		}
	}

	const auto new_cluster = WorldMap::get_cluster(new_coord);
	if(!new_cluster){
		LOG_EMPERY_CLUSTER_DEBUG("Lost connection to center server: new_coord = ", new_coord);
		result = Response(Msg::ERR_CLUSTER_CONNECTION_LOST) <<new_coord;
		return UINT64_MAX;
	}

	const auto new_cluster_scope = WorldMap::get_cluster_scope(new_coord);
	const auto map_x = static_cast<unsigned>(new_coord.x() - new_cluster_scope.left());
	const auto map_y = static_cast<unsigned>(new_coord.y() - new_cluster_scope.bottom());
	const auto cell_data = Data::MapCellBasic::require(map_x, map_y);
	const auto terrain_id = cell_data->terrain_id;
	const auto terrain_data = Data::MapTerrain::require(terrain_id);
	if(!terrain_data->passable){
		LOG_EMPERY_CLUSTER_DEBUG("Blocked by terrain: terrain_id = ", terrain_id);
		result = Response(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<terrain_id;
		return UINT64_MAX;
	}
	const unsigned border_thickness = Data::Global::as_unsigned(Data::Global::SLOT_MAP_BORDER_THICKNESS);
	if((map_x < border_thickness) || (map_x >= new_cluster_scope.width() - border_thickness) ||
		(map_y < border_thickness) || (map_y >= new_cluster_scope.height() - border_thickness))
	{
		LOG_EMPERY_CLUSTER_DEBUG("Blocked by map border: new_coord = ", new_coord);
		result = Response(Msg::ERR_BLOCKED_BY_IMPASSABLE_MAP_CELL) <<new_coord;
		return UINT64_MAX;
	}

	std::vector<boost::shared_ptr<MapObject>> adjacent_objects;
	WorldMap::get_map_objects_by_rectangle(adjacent_objects,
		Rectangle(Coord(new_coord.x() - 3, new_coord.y() - 3), Coord(new_coord.x() + 4, new_coord.y() + 4)));
	std::vector<Coord> foundation;
	for(auto it = adjacent_objects.begin(); it != adjacent_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_map_object_uuid = other_object->get_map_object_uuid();
		const auto other_coord = other_object->get_coord();
		if(new_coord == other_coord){
			LOG_EMPERY_CLUSTER_DEBUG("Blocked by another map object: other_map_object_uuid = ", other_map_object_uuid);
			if(!other_object->m_waypoints.empty()){
				const auto retry_max_count = get_config<unsigned>("blocked_path_retry_max_count", 10);
				const auto retry_delay = get_config<std::uint64_t>("blocked_path_retry_delay", 500);
				LOG_EMPERY_CLUSTER_DEBUG("Should we retry? blocked_retry_count = ", m_blocked_retry_count,
					", retry_max_count = ", retry_max_count, ", retry_delay = ", retry_delay);
				if(m_blocked_retry_count < retry_max_count){
					++m_blocked_retry_count;
					return retry_delay;
				}
				LOG_EMPERY_CLUSTER_DEBUG("Give up the path.");
			}
			result = Response(Msg::ERR_BLOCKED_BY_TROOPS) <<other_map_object_uuid;
			return UINT64_MAX;
		}
		const auto other_owner_uuid = other_object->get_owner_uuid();
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if((other_owner_uuid != owner_uuid) && (other_object_type_id == EmperyCenter::MapObjectTypeIds::ID_CASTLE)){
			foundation.clear();
			get_castle_foundation(foundation, other_coord, false);
			for(auto fit = foundation.begin(); fit != foundation.end(); ++fit){
				if(new_coord == *fit){
					LOG_EMPERY_CLUSTER_DEBUG("Blocked by castle: other_map_object_uuid = ", other_map_object_uuid);
					result = Response(Msg::ERR_BLOCKED_BY_CASTLE) <<other_map_object_uuid;
					return UINT64_MAX;
				}
			}
		}
	}
	set_coord(new_coord);

	m_waypoints.pop_front();
	m_blocked_retry_count = 0;
	
	if((m_action == ACT_ATTACK)&&(is_in_attack_scope(MapObjectUuid(m_action_param)))){
		//监测是否是在攻击范围之内
		const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
		if(!map_object_type_data){
			result = Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
			return delay;
		}
		const auto first_attack = map_object_type_data->first_attack*1000;
		m_waypoints.clear();
		return first_attack;
	}
	
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
void MapObject::set_attributes(const boost::container::flat_map<AttributeId, std::int64_t> &modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		m_attributes.emplace(it->first, 0);
	}

	bool dirty = false;
	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		auto &value = m_attributes.at(it->first);
		if(value == it->second){
			continue;
		}
		value = it->second;
		++dirty;
	}
	if(!dirty){
	    return;
	}

	WorldMap::update_map_object(virtual_shared_from_this<MapObject>(), false);
}

void MapObject::set_action(Coord from_coord, std::deque<Waypoint> waypoints, MapObject::Action action, std::string action_param){
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
}

boost::shared_ptr<AiControl>  MapObject::require_ai_control(){
	if(!m_ai_control){
		m_ai_control = boost::make_shared<AiControl>(virtual_weak_from_this<MapObject>());
	}
	return m_ai_control;
}

std::uint64_t MapObject::attack(std::pair<long, std::string> &result, std::uint64_t now){
	const auto target_object_uuid = MapObjectUuid(m_action_param);
	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		result = Response(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << get_map_object_type_id();
		return UINT64_MAX;
	}
	const auto target_object = WorldMap::get_map_object(target_object_uuid);
	if(!target_object){
		result = Response(Msg::ERR_NO_ATTACK_TARGT) << target_object_uuid;
		return UINT64_MAX;
	}
	const auto attack_speed = map_object_type_data->attack_speed * 1000;
	
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
	
	//直接发送到客户端
	Msg::SC_MapObjectAttack msgAttack;
	msgAttack.attacking_uuid  = m_map_object_uuid.str();
	msgAttack.attacked_uuid =  target_object_uuid.str();
	msgAttack.impact = IMPACT_NORMAL;
	msgAttack.damage = 0;
	msgAttack.x = target_object->get_coord().x();
	msgAttack.y = target_object->get_coord().y();
	
	bool bDodge = false;
	bool bCritical = false;
	std::uint64_t damage = 0;
	double addition_params = 1.0;//加成参数
	double damage_reduce_rate = 0.0;//伤害减免率
	double doge_rate = 0.30;
	double critical_rate = 0.30;
	double critical_demage_plus_rate = 0.30;
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	auto ememy_solider_count = target_object->get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	double relative_rate = Data::MapObjectRelative::get_relative(map_object_type_data->arm_type_id,emempy_type_data->arm_type_id);
	//计算闪避，闪避成功，
	bDodge = Poseidon::rand32()%100 < doge_rate*100;
	
	if(bDodge){
		msgAttack.impact = IMPACT_MISS;
		msgAttack.damage = 0;
	}else{
		//伤害计算
		if(target_object->get_map_object_type_id() == EmperyCenter::MapObjectTypeIds::ID_CASTLE ){
			damage = (1.0 +(soldier_count/10000.0) + (soldier_count - ememy_solider_count)/10000.0 * 0.5*relative_rate*
			pow((map_object_type_data->attack*addition_params),2)/(map_object_type_data->attack*addition_params + emempy_type_data->defence*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate));
		}else{
			damage =  (1.0 +(soldier_count/10000.0)*relative_rate*
			pow((map_object_type_data->attack*addition_params),2)/(map_object_type_data->attack*addition_params + emempy_type_data->defence*addition_params)*map_object_type_data->attack_plus*(1.0+damage_reduce_rate));
		}
		msgAttack.impact = IMPACT_NORMAL;
		msgAttack.damage = damage;
		//暴击计算
		bCritical = Poseidon::rand32()%100 < critical_rate*100;
		if(bCritical){
			msgAttack.impact = IMPACT_CRITICAL;
			damage = damage*(1.0+critical_demage_plus_rate);
			msgAttack.damage = damage;
		}
	}
	
	std::int64_t new_ememy_solider_count = ememy_solider_count - static_cast<std::int64_t>(damage);
	new_ememy_solider_count = (new_ememy_solider_count >= 0) ? new_ememy_solider_count : 0;
	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.emplace(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT, new_ememy_solider_count);
	target_object->set_attributes(std::move(modifiers));
	std::int64_t min_x,min_y,x,y,target_x,target_y;
	std::uint64_t width,height;
	x = get_coord().x();
	y = get_coord().y();
	target_x = target_object->get_coord().x();
	target_y = target_object->get_coord().y();
	min_x = (x < target_x) ? x:target_x;
	min_y = (y < target_y) ? y:target_y;
	width = static_cast<std::uint64_t>(abs(x-target_x));
	height = static_cast<std::uint64_t>(abs(y-target_y));
	const auto cluster = get_cluster();
	if(cluster){
		cluster->send_notification_by_rectangle(Rectangle(Coord(min_x,min_y), width, height),msgAttack);
	}
	
	
	//判断受攻击者是否死亡
	if(!target_object->is_die()){
		target_object->require_ai_control()->on_attack(virtual_shared_from_this<MapObject>(),damage);
	}else{
		target_object->require_ai_control()->on_die(virtual_shared_from_this<MapObject>());
	}
	return attack_speed;
}

std::uint64_t MapObject::on_attack(boost::shared_ptr<MapObject> attacker,std::uint64_t damage){
	//如果没有在攻击，则判断攻击者是否在自己的攻击范围之内，是则执行攻击，否则小范围内寻路攻击
	if(m_action != ACT_ATTACK && m_waypoints.empty() ){
		if(is_in_attack_scope(attacker->get_map_object_uuid())){
			set_action(get_coord(), m_waypoints, static_cast<MapObject::Action>(ACT_ATTACK),attacker->get_map_object_uuid().str());
		}else{
			
		}
	}
	return UINT64_MAX;
}

std::uint64_t MapObject::on_die(boost::shared_ptr<MapObject> attacker){
	WorldMap::remove_map_object(get_map_object_uuid());
	return UINT64_MAX;
}

bool MapObject::is_die(){
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	return (soldier_count > 0 ) ? false:true;
}

bool MapObject::is_in_attack_scope(MapObjectUuid target_object_uuid){
	const auto map_object_type_data = Data::MapObjectType::get(get_map_object_type_id());
	if(!map_object_type_data){
		return false;
	}
	const auto shoot_range = map_object_type_data->shoot_range;
	
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

}
