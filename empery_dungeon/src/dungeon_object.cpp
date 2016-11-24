#include "precompiled.hpp"
#include "dungeon_object.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "dungeon_client.hpp"
#include "cbpp_response.hpp"
#include "dungeon_utilities.hpp"
#include "mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/random.hpp>
#include "../../empery_center/src/msg/sc_dungeon.hpp"
#include "../../empery_center/src/msg/sd_dungeon.hpp"
#include "../../empery_center/src/msg/ds_dungeon.hpp"
#include "../../empery_center/src/msg/err_dungeon.hpp"
#include "../../empery_center/src/msg/err_map.hpp"
#include "../../empery_center/src/msg/err_castle.hpp"
#include "../../empery_center/src/attribute_ids.hpp"
#include "src/data/skill.hpp"
#include "src/data/dungeon.hpp"
namespace EmperyDungeon {



namespace Msg = ::EmperyCenter::Msg;

DungeonObject::DungeonObject(DungeonUuid dungeon_uuid, DungeonObjectUuid dungeon_object_uuid,
	DungeonObjectTypeId dungeon_object_type_id, AccountUuid owner_uuid,Coord coord,std::string tag)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_object_uuid(dungeon_object_uuid)
	, m_dungeon_object_type_id(dungeon_object_type_id), m_owner_uuid(owner_uuid)
	, m_coord(coord),m_tag(tag)
{
}
DungeonObject::~DungeonObject(){
}

void DungeonObject::set_current_skill(DungeonMonsterSkillId skill_id,Coord coord,std::string param){
	PROFILE_ME;

	m_current_skill_id = skill_id;
	m_skill_target_coord = coord;
	m_skill_param = param;
	auto it = m_skills.find(skill_id);
	if(it != m_skills.end()){
		auto &skill = it->second;
		skill->set_cast_coord(coord);
		skill->set_cast_params(param);
	}
}


std::uint64_t DungeonObject::pump_action(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	//判断副本中暂停状态
	const auto dungeon = DungeonMap::require(m_dungeon_uuid);
	if(dungeon->get_fight_state() == Dungeon::FIGHT_PAUSE){
		LOG_EMPERY_DUNGEON_DEBUG("dungeon in fight pause...");
		const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
		return stand_by_interval;
	}

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_DUNGEON_OBJECT_TYPE) << get_dungeon_object_type_id();
		return UINT64_MAX;
	}

	if(is_lost_attacked_target()){
		LOG_EMPERY_DUNGEON_DEBUG("lost attacked target,and prepare search attack,current action:",m_action);
		return search_attack();
	}

	//修正action
	if(!fix_attack_action(result)){
		LOG_EMPERY_DUNGEON_DEBUG("fix attack action failed,action cancel,current action:",m_action);
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
		return require_ai_control()->on_action_guard(result,now);
	}
	ON_ACTION(ACT_ATTACK){
		// TODO 战斗。
		return require_ai_control()->on_action_attack(result,now);
	}
	ON_ACTION(ACT_MONTER_REGRESS){
		//野怪回归
		LOG_EMPERY_DUNGEON_DEBUG("monster regress finish,cancel action",m_action);
		return require_ai_control()->on_action_monster_regress(result,now);
	}
	ON_ACTION(ACT_MONSTER_SEARCH_TARGET){
		return require_ai_control()->on_action_monster_search_target(result,now);
	}
	ON_ACTION(ACT_MONSTER_PATROL){
		return require_ai_control()->on_action_patrol(result,now);
	}
	ON_ACTION(ACT_SKILL_SING){
		return require_ai_control()->on_action_skill_singing(result,now);
	}
	ON_ACTION(ACT_SKILL_CAST){
		return require_ai_control()->on_action_skill_casting(result,now);
	}
//=============================================================================
#undef ON_ACTION
		}
		break;
	default:
		LOG_EMPERY_DUNGEON_WARNING("Unknown action: action = ", static_cast<unsigned>(m_action));
		result = CbppResponse(Msg::ERR_UNKNOWN_MAP_OBJECT_ACTION) <<static_cast<unsigned>(m_action);
		break;
	}
	return UINT64_MAX;
}

std::int64_t DungeonObject::get_attribute(AttributeId attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(attribute_id);
	if(it == m_attributes.end()){
		return 0;
	}
	return it->second;
}
void DungeonObject::get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second;
	}
}
void DungeonObject::set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers){
	PROFILE_ME;

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			m_attributes.emplace(it->first, 0);
		}
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

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
	}
}

DungeonObject::BuffInfo DungeonObject::get_buff(BuffId buff_id) const{
	PROFILE_ME;

	BuffInfo info = { };
	info.buff_id = buff_id;
	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return info;
	}
	if(it->second.time_end == 0){
		return info;
	}
	info = it->second;
	return info;
}

bool DungeonObject::is_buff_in_effect(BuffId buff_id) const{
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

void DungeonObject::get_buffs(std::vector<DungeonObject::BuffInfo> &ret) const{
	PROFILE_ME;

	ret.reserve(ret.size() + m_buffs.size());
	for(auto it = m_buffs.begin(); it != m_buffs.end(); ++it){
		if(it->second.time_end == 0){
			continue;
		}
		ret.emplace_back(it->second);
	}
}

void DungeonObject::set_buff(BuffId buff_id, std::uint64_t duration){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	set_buff(buff_id, utc_now, duration);
}

void DungeonObject::set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration){
	PROFILE_ME;

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

void DungeonObject::clear_buff(BuffId buff_id) noexcept{
	PROFILE_ME;

	const auto it = m_buffs.find(buff_id);
	if(it == m_buffs.end()){
		return;
	}
	m_buffs.erase(it);
}

void DungeonObject::set_action(Coord from_coord, std::deque<std::pair<signed char, signed char>> waypoints,DungeonObject::Action action, std::string action_param){
	PROFILE_ME;

	const auto timer_proc = [this](const boost::weak_ptr<DungeonObject> &weak, std::uint64_t now){
		PROFILE_ME;

		const auto shared = weak.lock();
		if(!shared){
			return;
		}
		const auto dungeon_object_uuid = shared->get_dungeon_object_uuid();
		LOG_EMPERY_DUNGEON_TRACE("Dungeon object action timer: dungeon_object_uuid = ", dungeon_object_uuid);
		const auto dungeon_uuid = shared->get_dungeon_uuid();
		const auto dungeon = DungeonMap::require(dungeon_uuid);

		for(;;){
			if(now < m_next_action_time){
				/*
				if(m_action_timer){
					Poseidon::TimerDaemon::set_absolute_time(m_action_timer, m_next_action_time);
				}
				*/
				break;
			}
			std::uint64_t delay = UINT64_MAX;
			std::pair<long, std::string> result;
			try {
				delay = pump_action(result, now);
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				result.first = Msg::ERR_INVALID_ACTION_PARAM;
				try {
					result.second = e.what();
				} catch(std::exception &e){
					LOG_EMPERY_DUNGEON_ERROR("std::exception thrown: what = ", e.what());
				}
			}
			if(delay == UINT64_MAX){
				auto old_action = m_action;
				auto old_param  = std::move(m_action_param);

				LOG_EMPERY_DUNGEON_DEBUG("Action stopped: dungeon_object_uuid = ", dungeon_object_uuid,
					", error_code = ", result.first, ", error_message = ", result.second);
				m_waypoints.clear();
				m_action = ACT_GUARD;
				m_action_param.clear();

				LOG_EMPERY_DUNGEON_DEBUG("Releasing action timer: dungeon_object_uuid = ", dungeon_object_uuid);
				m_action_timer.reset();

				const auto dungeon_client = dungeon->get_dungeon_client();
				if(dungeon_client){
					try {
						Msg::DS_DungeonObjectStopped msg;
						msg.dungeon_uuid        = dungeon_uuid.str();
						msg.dungeon_object_uuid = dungeon_object_uuid.str();
						msg.action              = old_action;
						msg.param               = std::move(old_param);
						msg.error_code          = result.first;
						msg.error_message       = std::move(result.second);
						dungeon_client->send(msg);
					} catch(std::exception &e){
						LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
						dungeon_client->shutdown(e.what());
					}
					dungeon->update_object(virtual_shared_from_this<DungeonObject>(), false);
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
				std::bind(timer_proc, virtual_weak_from_this<DungeonObject>(), std::placeholders::_2));
			LOG_EMPERY_DUNGEON_DEBUG("Created action timer: dungeon_object_uuid = ", get_dungeon_object_uuid());
			m_action_timer = std::move(timer);
		}
		if(m_next_action_time < now){
			m_next_action_time = now;
		}
	}
	m_waypoints    = std::move(waypoints);
	m_action       = action;
	m_action_param = std::move(action_param);
	notify_way_points(m_waypoints,action,m_action_param);
	reset_attack_target_own_uuid();
}

bool DungeonObject::is_die(){
	PROFILE_ME;

	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
	return (soldier_count > 0 ) ? false:true;
}
bool DungeonObject::is_in_attack_scope(boost::shared_ptr<DungeonObject> target_object){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		return false;
	}
	const auto shoot_range = get_shoot_range();
	if(!target_object){
		return false;
	}

	const auto coord    = get_coord();
	const auto distance = get_distance_of_coords(coord, target_object->get_coord());
	if(distance <= shoot_range){
		return true;
	}
	return false;
}
bool DungeonObject::is_in_group_view_scope(boost::shared_ptr<DungeonObject>& target_object){
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
std::uint64_t DungeonObject::get_view_range(){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		return 0;
	}
	return dungeon_object_type_data->view + get_attribute(EmperyCenter::AttributeIds::ID_SIGHT_RANGE_ADD) / 1000.0;
}
std::uint64_t DungeonObject::get_shoot_range(){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		return 0;
	}
	const auto shoot_range = dungeon_object_type_data->shoot_range + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_RANGE_ADD) / 1000.0;
	return shoot_range;
}

bool DungeonObject::get_new_enemy(AccountUuid owner_uuid,boost::shared_ptr<DungeonObject> &new_enemy_dungeon_object){
	PROFILE_ME;

	if(get_owner_uuid() == owner_uuid){
		return false;
	}
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		return false;
	}
	std::vector<boost::shared_ptr<DungeonObject>> dungeon_objects;
	dungeon->get_dungeon_objects_by_account(dungeon_objects,owner_uuid);
	auto view_range = get_view_range();
	auto min_distance = UINT64_MAX;
	boost::shared_ptr<DungeonObject> select_object;
	for(auto it = dungeon_objects.begin(); it != dungeon_objects.end(); ++it){
		const auto &dungeon_object = *it;
		std::pair<long, std::string> reason;
		if(!dungeon_object->attacked_able(reason)){
			continue;
		}
		auto distance = get_distance_of_coords(get_coord(),dungeon_object->get_coord());
		if(distance> view_range){
			continue;
		}
		if(distance < min_distance){
			min_distance = distance;
			select_object = dungeon_object;
		}else if(distance == min_distance){
			if(select_object->get_attacked_prority() > dungeon_object->get_attacked_prority()){
				select_object = dungeon_object;
			}
		}
	}
	if(min_distance != UINT64_MAX){
		new_enemy_dungeon_object = select_object;
		return true;
	}
	return false;
}

bool DungeonObject::get_monster_new_enemy(boost::shared_ptr<DungeonObject> &new_enemy_dungeon_object){
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		return false;
	}
	std::vector<boost::shared_ptr<DungeonObject>> dungeon_objects;
	dungeon->get_objects_all(dungeon_objects);
	auto view_range = get_view_range();
	auto min_distance = UINT64_MAX;
	boost::shared_ptr<DungeonObject> select_object;
	for(auto it = dungeon_objects.begin(); it != dungeon_objects.end(); ++it){
		const auto &dungeon_object = *it;
		std::pair<long, std::string> reason;
		if(!dungeon_object->attacked_able(reason)){
			continue;
		}
		if(dungeon_object->is_monster()){
			continue;
		}
		auto distance = get_distance_of_coords(get_coord(),dungeon_object->get_coord());
		if(distance> view_range){
			continue;
		}
		if(distance < min_distance){
			min_distance = distance;
			select_object = dungeon_object;
		}else if(distance == min_distance){
			if(select_object->get_attacked_prority() > dungeon_object->get_attacked_prority()){
				select_object = dungeon_object;
			}
		}
	}
	if(min_distance != UINT64_MAX){
		new_enemy_dungeon_object = select_object;
		return true;
	}
	return false;
}

void DungeonObject::attack_new_target(boost::shared_ptr<DungeonObject> enemy_dungeon_object){
	PROFILE_ME;

	if(!enemy_dungeon_object)
		return;

	std::pair<long, std::string> reason;
	if(!enemy_dungeon_object->attacked_able(reason)){
		return;
	}

	if(is_in_attack_scope(enemy_dungeon_object)){
			set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_ATTACK),enemy_dungeon_object->get_dungeon_object_uuid().str());
		}else{
			std::deque<std::pair<signed char, signed char>> waypoints;
			if(find_way_points(waypoints,get_coord(),enemy_dungeon_object->get_coord(),false)){
				set_action(get_coord(), waypoints, static_cast<DungeonObject::Action>(ACT_ATTACK),enemy_dungeon_object->get_dungeon_object_uuid().str());
			}else{
				set_action(get_coord(), waypoints, static_cast<DungeonObject::Action>(ACT_GUARD),"");
			}
	}
}
bool DungeonObject::attacked_able(std::pair<long, std::string> &reason){
	PROFILE_ME;

	if(is_monster() && (m_action == ACT_MONTER_REGRESS)){
		reason = CbppResponse(Msg::ERR_TEMPORARILY_INVULNERABLE);
		return false;
	}
	const auto ai_control = boost::dynamic_pointer_cast<AiControlMonsterDecorate>(require_ai_control());
	if(ai_control){
		reason = CbppResponse(Msg::ERR_DUNGEON_MONSTER_DECORATE_AI);
		return false;
	}
	return true;
}
bool DungeonObject::attacking_able(std::pair<long, std::string> &reason){
	if(is_die()){
		reason = CbppResponse(Msg::ERR_ZERO_SOLDIER_COUNT);
		return false;
	}
	//攻击力为0不可以攻击(副本中物件)
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(dungeon_object_type_data->attack == 0){
		reason = CbppResponse(Msg::ERR_ZERO_DUNGEON_OBJECT_ATTACK)<< get_dungeon_object_type_id();
		return false;
	}
	const auto ai_control = boost::dynamic_pointer_cast<AiControlMonsterDecorate>(require_ai_control());
	if(ai_control){
		reason = CbppResponse(Msg::ERR_DUNGEON_MONSTER_DECORATE_AI) << get_dungeon_object_type_id();
		return false;
	}
	return true;
}
std::uint64_t DungeonObject::search_attack(){
	boost::shared_ptr<DungeonObject> near_enemy_object;
	bool is_get_new_enemy = get_new_enemy(get_attack_target_own_uuid(),near_enemy_object);
	if(is_get_new_enemy){
		attack_new_target(near_enemy_object);
	}else{
		require_ai_control()->on_lose_target();
	}
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

boost::shared_ptr<AiControl>  DungeonObject::require_ai_control(){
	PROFILE_ME;

	if(!m_ai_control){
		const auto dungeon_object_type_data = get_dungeon_object_type_data();
		const auto ai_id   = dungeon_object_type_data->ai_id;
		const auto ai_data = get_dungeon_ai_data();
		const auto ai_type = ai_data->ai_type;
		switch(ai_type){
			case AI_SOLIDER:
				m_ai_control = boost::make_shared<AiControl>(ai_id,virtual_weak_from_this<DungeonObject>());
				break;
			case AI_MONSTER_AUTO_SEARCH_TARGET:
				m_ai_control = boost::make_shared<AiControlMonsterAutoSearch>(ai_id,virtual_weak_from_this<DungeonObject>());
				break;
			case AI_MONSTER_PATROL:
				m_ai_control = boost::make_shared<AiControlMonsterPatrol>(ai_id,virtual_weak_from_this<DungeonObject>());
				break;
			case AI_MONSTER_OBJECT:
				m_ai_control = boost::make_shared<AiControlMonsterObject>(ai_id,virtual_weak_from_this<DungeonObject>());
				break;
			case AI_MONSTER_DECORATE:
				m_ai_control = boost::make_shared<AiControlMonsterDecorate>(ai_id,virtual_weak_from_this<DungeonObject>());
				break;
			default:
				LOG_EMPERY_DUNGEON_WARNING("invalid ai type:",ai_data->ai_type," ai_id:",ai_id);
				break;
		}
	}
	return m_ai_control;
}

std::uint64_t DungeonObject::move(std::pair<long, std::string> &result){
	PROFILE_ME;

	const auto owner_uuid      = get_owner_uuid();
	const auto coord           = get_coord();

	const auto waypoint  = m_waypoints.front();
	const auto new_coord = Coord(coord.x() + waypoint.first, coord.y() + waypoint.second);
	const auto dungeon = DungeonMap::require(get_dungeon_uuid());

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	std::uint64_t delay;
	const auto speed = get_move_speed();
	if(speed <= 0){
		LOG_EMPERY_DUNGEON_WARNING("speed <= 0,",speed, " dungeon_object_type_data->speed:",dungeon_object_type_data->speed," get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0",get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0,
		" get_attribute(EmperyCenter::AttributeIds::ID_SPEED_ADD):",get_attribute(EmperyCenter::AttributeIds::ID_SPEED_ADD) / 1000.0);
		delay = UINT64_MAX;
	} else {
		delay = static_cast<std::uint64_t>(std::round(1000 / speed));
	}
	const auto retry_max_count = get_config<unsigned>("blocked_path_retry_max_count", 10);
	const auto wait_for_moving_objects = (m_blocked_retry_count < retry_max_count);
	result = get_move_result(dungeon->get_dungeon_uuid(),new_coord, owner_uuid, wait_for_moving_objects);
	if(result.first == Msg::ERR_BLOCKED_BY_TROOPS_TEMPORARILY){
		const auto retry_delay = get_config<std::uint64_t>("blocked_path_retry_delay", 500);
		++m_blocked_retry_count;
		return retry_delay;
	}
	m_blocked_retry_count = 0;

	if(result.first != Msg::ST_OK){
		// XXX 根据不同 action 计算路径。
		const auto to_coord = std::accumulate(m_waypoints.begin(), m_waypoints.end(), coord,
			[](Coord c, std::pair<signed char, signed char> d){ return Coord(c.x() + d.first, c.y() + d.second); });
		std::deque<std::pair<signed char, signed char>> new_waypoints;
		if(find_way_points(new_waypoints, coord, to_coord, true)|| !new_waypoints.empty()){
			notify_way_points(new_waypoints, m_action, m_action_param);
			m_waypoints = std::move(new_waypoints);
			return 0;
		}
		return UINT64_MAX;
	}

	set_coord(new_coord);
	m_waypoints.pop_front();
	m_blocked_retry_count = 0;
	dungeon->check_move_pass(new_coord,get_dungeon_object_uuid().str(),is_monster());
	return delay;
}

void DungeonObject::set_coord(Coord coord){
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;
	const auto dungeon = DungeonMap::require(get_dungeon_uuid());
	dungeon->update_object(virtual_shared_from_this<DungeonObject>(),false);
}

/*
攻击：
1.用户发起攻击（部队）
2.由于主动被动联动导致攻击
3.丢失目标重新寻找攻击
4.野怪主动攻击（野怪）
5.攻击时重新选择就近目标
*/
std::uint64_t DungeonObject::attack(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto target_object_uuid = DungeonObjectUuid(m_action_param);
	if(get_dungeon_object_uuid() == target_object_uuid){
		result = CbppResponse(Msg::ERR_CANNOT_ATTACK_FRIENDLY_OBJECTS) << get_dungeon_object_uuid();
		return UINT64_MAX;
	}
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_DUNGEON_OBJECT_TYPE) << get_dungeon_object_type_id();
		return UINT64_MAX;
	}
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		result = CbppResponse(Msg::ERR_NO_SUCH_DUNGEON) << get_dungeon_uuid();
		return UINT64_MAX;
	}
	const auto target_object = dungeon->get_object(target_object_uuid);
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
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(!dungeon_client){
		result = CbppResponse(Msg::ERR_DUNGEON_SERVER_CONNECTION_LOST) <<dungeon->get_dungeon_uuid();
		return UINT64_MAX;
	}
	const auto emempy_type_data = target_object->get_dungeon_object_type_data();
	if(!emempy_type_data){
		result = CbppResponse(Msg::ERR_NO_SUCH_MAP_OBJECT_TYPE) << target_object->get_dungeon_object_type_id();
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
	double k = 0.06;
	double doge_rate = emempy_type_data->doge_rate + get_attribute(EmperyCenter::AttributeIds::ID_DODGING_RATIO_ADD)/ 1000.0 + dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_DODGING_RATIO_ADD) / 1000.0;
	double critical_rate = dungeon_object_type_data->critical_rate + get_attribute(EmperyCenter::AttributeIds::ID_CRITICAL_DAMAGE_RATIO_ADD) / 1000.0 + dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_CRITICAL_DAMAGE_RATIO_ADD) / 1000.0;
	double critical_demage_plus_rate = dungeon_object_type_data->critical_damage_plus_rate + get_attribute(EmperyCenter::AttributeIds::ID_CRITICAL_DAMAGE_MULTIPLIER_ADD) / 1000.0 + dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_CRITICAL_DAMAGE_MULTIPLIER_ADD) / 1000.0;
	//double total_attack  = dungeon_object_type_data->attack * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0) + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_ADD) / 1000.0;
	//double total_defense = emempy_type_data->defence * (1.0 + target_object->get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BONUS) / 1000.0) + target_object->get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_ADD) / 1000.0;
	double total_attack  = get_total_attack();
	double total_defense = target_object->get_total_defense();
	double relative_rate = Data::DungeonObjectRelative::get_relative(get_arm_attack_type(),target_object->get_arm_defence_type());
//	std::uint32_t hp =  dungeon_object_type_data->hp ;
//	hp = (hp == 0 )? 1:hp;
	auto soldier_count = get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
//	if(soldier_count%hp != 0){
//		soldier_count = (soldier_count/hp + 1)*hp;
//	}
	//auto ememy_solider_count = target_object->get_attribute(EmperyCenter::AttributeIds::ID_SOLDIER_COUNT);
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
	//腐蚀buff
	double corrosion = dungeon->get_dungeon_attribute(target_object->get_dungeon_object_uuid(),target_object->get_owner_uuid(),EmperyCenter::AttributeIds::ID_CORROSION) / 1000.0;
	if(corrosion > 0.0){
		LOG_EMPERY_DUNGEON_FATAL("attacked have a corrosion buff ,damge before:",damage," corrosion = ",corrosion);
		damage = damage*( 1.0 + corrosion);
		LOG_EMPERY_DUNGEON_FATAL("attacked have a corrosion buff ,damge end :",damage);
	}

	//防御矩阵buff
	double defense_matrix = dungeon->get_dungeon_attribute(target_object->get_dungeon_object_uuid(),target_object->get_owner_uuid(),EmperyCenter::AttributeIds::ID_DEFENSE_MATRIX) / 1000.0;
	if(defense_matrix > 0.0){
		damage = damage*(1 - defense_matrix);
		LOG_EMPERY_DUNGEON_FATAL("matrix damage");
	}

	Msg::DS_DungeonObjectAttackAction msg;
	msg.dungeon_uuid = get_dungeon_uuid().str();
	msg.attacking_account_uuid = get_owner_uuid().str();
	msg.attacking_object_uuid = get_dungeon_object_uuid().str();
	msg.attacking_object_type_id = get_dungeon_object_type_id().get();
	msg.attacking_coord_x = get_coord().x();
	msg.attacking_coord_y = get_coord().y();
	msg.attacked_account_uuid = target_object->get_owner_uuid().str();
	msg.attacked_object_uuid = target_object->get_dungeon_object_uuid().str();
	msg.attacked_object_type_id = target_object->get_dungeon_object_type_id().get();
	msg.attacked_coord_x = target_object->get_coord().x();
	msg.attacked_coord_y = target_object->get_coord().y();
	msg.result_type = result_type;
	msg.soldiers_damaged = damage;
	auto sresult = dungeon_client->send_and_wait(msg);
	if(sresult.first != Msg::ST_OK){
		LOG_EMPERY_DUNGEON_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
		result = std::move(sresult);
		return UINT64_MAX;
	}
	//反伤
	if(target_object->can_reflex_injury()){
		target_object->do_reflex_injury(damage,virtual_shared_from_this<DungeonObject>());
	}

	//判断受攻击者是否死亡
	if(!target_object->is_die()){
		target_object->require_ai_control()->on_attack(virtual_shared_from_this<DungeonObject>(),damage);
		require_ai_control()->troops_attack(target_object);
	}

	std::uint64_t attack_delay = get_attack_delay();
	return attack_delay;
}

/*
1:玩家部队均可联动
2：野怪配置可以联动接受联动
3：副本中物件攻击力为0，ai配置成不可以联动
*/
void DungeonObject::troops_attack(boost::shared_ptr<DungeonObject> target, bool passive){
	PROFILE_ME;

	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		LOG_EMPERY_DUNGEON_DEBUG("no find the parent dungeon, dengeon_uuid = ", get_dungeon_uuid());
		return;
	}
	std::vector<boost::shared_ptr<DungeonObject>> friendly_dungeon_objects;
	dungeon->get_dungeon_objects_by_account(friendly_dungeon_objects,get_owner_uuid());
	if(friendly_dungeon_objects.empty()){
		return;
	}
	if(!target){
		LOG_EMPERY_DUNGEON_DEBUG("target dungeon object is null find");
		return;
	}
	for(auto it = friendly_dungeon_objects.begin(); it != friendly_dungeon_objects.end(); ++it){
		auto dungeon_object = *it;
		std::pair<long, std::string> reason;
		if(!dungeon_object || !dungeon_object->is_idle() || !is_in_group_view_scope(dungeon_object) || !dungeon_object->attacking_able(reason)
			|| (dungeon_object->get_dungeon_object_uuid() == get_dungeon_object_uuid())){
			continue;
		}
		if(dungeon_object->is_monster()){
			const auto ai_data = dungeon_object->get_dungeon_ai_data();
			if(!ai_data->ai_linkage){
				continue;
			}
		}
		boost::shared_ptr<DungeonObject> near_enemy_object;
		if(passive&&dungeon_object->get_new_enemy(target->get_owner_uuid(),near_enemy_object)){
			dungeon_object->attack_new_target(near_enemy_object);
		}else{
			dungeon_object->attack_new_target(target);
		}
	}
}
std::uint64_t DungeonObject::on_attack_common(boost::shared_ptr<DungeonObject> attacker,std::uint64_t demage){
	PROFILE_ME;

	if(!attacker){
		return UINT64_MAX;
	}
	if(m_action != ACT_ATTACK && m_waypoints.empty() && m_action != ACT_SKILL_SING && m_action != ACT_SKILL_CAST){
		attack_new_target(attacker);
	}
	require_ai_control()->troops_attack(attacker,true);
	return UINT64_MAX;
}
std::uint64_t DungeonObject::lost_target_common(){
	set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_GUARD),"");
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}
std::uint64_t DungeonObject::lost_target_monster(){
	monster_regress();
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);
	return stand_by_interval;
}

bool DungeonObject::fix_attack_action(std::pair<long, std::string> &result){
	PROFILE_ME;

	if( m_action != ACT_ATTACK){
		return true;
	}
	if(!attacking_able(result)){
		return false;
	}
	Coord target_coord;
	bool in_attack_scope = false;
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		LOG_EMPERY_DUNGEON_DEBUG("no find the parent dungeon, dengeon_uuid = ", get_dungeon_uuid());
		return false;
	}
	const auto target_object = dungeon->get_object(DungeonObjectUuid(m_action_param));
	if(!target_object){
		result = CbppResponse(Msg::ERR_ATTACK_TARGET_LOST);
		return false;
	}
	target_coord = target_object->get_coord();
	in_attack_scope = is_in_attack_scope(target_object);
	if(in_attack_scope&&!m_waypoints.empty()){
		m_waypoints.clear();
		notify_way_points(m_waypoints,m_action,m_action_param);
	}
	//如果野怪设置了就近攻击，则寻找视野范围内的敌军
	//有则直接攻击
	if(!in_attack_scope){
		const auto ai_data = get_dungeon_ai_data();
		if(ai_data->ai_Intelligence){
			boost::shared_ptr<DungeonObject> near_enemy_object;
			bool is_get_new_enemy = get_new_enemy(target_object->get_owner_uuid(),near_enemy_object);
			if(is_get_new_enemy && near_enemy_object){
				std::deque<std::pair<signed char, signed char>> waypoints;
				bool is_find_way = find_way_points(waypoints,get_coord(),near_enemy_object->get_coord());
				if(is_find_way && waypoints.size() < m_waypoints.size()){
					LOG_EMPERY_DUNGEON_ERROR("ATTACK NERAR ATTACK NEAR !!!!");
					attack_new_target(near_enemy_object);
					return true;
				}
			}
		}
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
bool DungeonObject::find_way_points(std::deque<std::pair<signed char, signed char>> &waypoints,Coord from_coord,Coord target_coord,bool precise){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
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
	const auto distance_limit = get_config<unsigned>("path_recalculation_radius", 20);
	bool result = find_path(path,from_coord, target_coord,get_dungeon_uuid(),get_owner_uuid(), distance_limit, distance_close_enough);
	for(auto it = path.begin(); it != path.end(); ++it){
		waypoints.emplace_back(it->first, it->second);
	}
	return result;
}

void DungeonObject::monster_regress(){
	PROFILE_ME;

	auto birth_x = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_X);
	auto birth_y = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_Y);
	std::deque<std::pair<signed char, signed char>> waypoints;
	if(find_way_points(waypoints,get_coord(),Coord(birth_x,birth_y),true)){
		set_action(get_coord(), waypoints, static_cast<DungeonObject::Action>(ACT_MONTER_REGRESS),"");
	}else{
		set_action(get_coord(), waypoints, static_cast<DungeonObject::Action>(ACT_GUARD),"");
	}
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		LOG_EMPERY_DUNGEON_DEBUG("server can't find the dungeon, dungeon_uuid = ", get_dungeon_uuid());
		DEBUG_THROW(Exception, sslit("server can't find the dungeon"));
	}

	const auto dungeon_client = dungeon->get_dungeon_client();
	if(dungeon_client){
		try {
			/*
			Msg::KS_MapHealMonster msg;
			msg.map_object_uuid = get_map_object_uuid().str();
			dungeon_client->send(msg);
			*/
		} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
			dungeon_client->shutdown(e.what());
		}
	}
}

bool DungeonObject::is_monster(){
	PROFILE_ME;

	const auto dungeon_object_type_monster_data = Data::DungeonObjectTypeMonster::get(get_dungeon_object_type_id());
	if(!dungeon_object_type_monster_data){
		return false;
	}
	return true;
}
bool DungeonObject::is_lost_attacked_target(){
	PROFILE_ME;
	if(m_action != ACT_ATTACK){
		return false;
	}
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		return false;
	}
	const auto target_object = dungeon->get_object(DungeonObjectUuid(m_action_param));
	if(!target_object){
		return true;
	}
	return false;
}
void DungeonObject::reset_attack_target_own_uuid(){
	PROFILE_ME;

	if(m_action != ACT_ATTACK){
		m_target_own_uuid = {};
		return;
	}
	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		LOG_EMPERY_DUNGEON_DEBUG("no find the parent dungeon, dungeon_uuid = ", get_dungeon_uuid());
		return;
	}

	const auto target_object = dungeon->get_object(DungeonObjectUuid(m_action_param));
	if(!target_object || target_object->is_monster()){
		m_target_own_uuid = {};
		return;
	}
	m_target_own_uuid = target_object->get_owner_uuid();
}

AccountUuid DungeonObject::get_attack_target_own_uuid(){
	return m_target_own_uuid;
}
unsigned DungeonObject::get_arm_attack_type(){
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_DEBUG("No dungeon object type data,id = ",get_dungeon_object_type_id());
		return 0;
	}
	return dungeon_object_type_data->attack_type;
}
unsigned DungeonObject::get_arm_defence_type(){
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_DEBUG("No dungeon object type data,id = ",get_dungeon_object_type_id());
		return 0;
	}
	return dungeon_object_type_data->defence_type;
}

int DungeonObject::get_attacked_prority(){
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		return 0;
	}
	int prority = 0;
	switch(dungeon_object_type_data->category_id.get()){
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

bool DungeonObject::move_able(){
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		return false;
	}
	if(dungeon_object_type_data->speed > 0){
		return true;
	}
	return false;
}

boost::shared_ptr<const Data::DungeonObjectType> DungeonObject::get_dungeon_object_type_data(){
	PROFILE_ME;

	return Data::DungeonObjectType::get(get_dungeon_object_type_id());
}

boost::shared_ptr<const Data::DungeonObjectAi>   DungeonObject::get_dungeon_ai_data(){
	PROFILE_ME;
	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	const auto ai_id   = dungeon_object_type_data->ai_id;
	return	Data::DungeonObjectAi::require(ai_id);
}

void   DungeonObject::notify_way_points(const std::deque<std::pair<signed char, signed char>> &waypoints,const DungeonObject::Action &action, const std::string &action_param){
	PROFILE_ME;

	const auto dungeon = DungeonMap::get(get_dungeon_uuid());
	if(!dungeon){
		return;
	}
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(dungeon_client){
		try {
			Msg::DS_DungeonWaypointsSet msg;
			msg.dungeon_uuid        = get_dungeon_uuid().str();
			msg.dungeon_object_uuid = get_dungeon_object_uuid().str();
			msg.x                   = get_coord().x();
			msg.y                   = get_coord().y();
			msg.waypoints.reserve(waypoints.size());
			for(auto it = waypoints.begin(); it != waypoints.end(); ++it){
				auto &waypoint = *msg.waypoints.emplace(msg.waypoints.end());
				waypoint.dx = it->first;
				waypoint.dy = it->second;
			}
			msg.action          = static_cast<unsigned>(action);
			msg.param           = action_param;
			if(m_action == ACT_ATTACK){
				const auto target_object_uuid = DungeonObjectUuid(m_action_param);
				const auto target_object = dungeon->get_object(target_object_uuid);
				if(target_object){
					msg.target_x        = target_object->get_coord().x();
					msg.target_y        = target_object->get_coord().y();
				}
			}
			LOG_EMPERY_DUNGEON_DEBUG("DS_DungeonWaypointsSet = ",msg);
			dungeon_client->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
			dungeon_client->shutdown(e.what());
		}
	}
}

std::uint64_t DungeonObject::on_monster_regress(){
	PROFILE_ME;

	//to ACT_MONSTER_SEARCH_TARGET
	set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_SEARCH_TARGET),"");
	const auto ai_data = get_dungeon_ai_data();
	return boost::lexical_cast<std::uint64_t>(ai_data->params)*1000;
}
std::uint64_t DungeonObject::monster_search_attack_target(std::pair<long, std::string> &result){
	PROFILE_ME;

	boost::shared_ptr<DungeonObject> new_enemy_dungeon_object;
	bool is_get_new_enemy = get_monster_new_enemy(new_enemy_dungeon_object);
	if(is_get_new_enemy && new_enemy_dungeon_object){
		attack_new_target(new_enemy_dungeon_object);
	}else{
		result = CbppResponse(Msg::ERR_FAIL_SEARCH_TARGE) << get_dungeon_object_uuid();
	}
	const auto ai_data = get_dungeon_ai_data();
	return boost::lexical_cast<std::uint64_t>(ai_data->params)*1000;
}
std::uint64_t DungeonObject::on_monster_guard(){
	PROFILE_ME;

	set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_SEARCH_TARGET),"");
	const auto ai_data = get_dungeon_ai_data();
	return boost::lexical_cast<std::uint64_t>(ai_data->params)*1000;
}

std::uint64_t DungeonObject::on_monster_patrol(){
	PROFILE_ME;

	auto birth_x = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_X);
	auto birth_y = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_START_POINT_Y);
	auto dest_x  = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_PATROL_DEST_POINT_X);
	auto dest_y  = get_attribute(EmperyCenter::AttributeIds::ID_MONSTER_PATROL_DEST_POINT_Y);
	Coord coord_birth(birth_x,birth_y);
	Coord coord_dest(dest_x,dest_y);
	const auto ai_data = get_dungeon_ai_data();
	if(m_waypoints.empty() && (coord_birth == get_coord())){
		//TODO goto the dest
		if(find_way_points(m_waypoints,get_coord(),coord_dest,true)){
			set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_PATROL),"");
			return boost::lexical_cast<std::uint64_t>(ai_data->params2)*1000;
		}else{
			LOG_EMPERY_DUNGEON_WARNING("find the way point to patrol dest fail，dungeon_uuid = ",get_dungeon_uuid());
			set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_SEARCH_TARGET),"");
		}
	}else if(m_waypoints.empty() && (coord_dest == get_coord())){
		//TODO goto the birth
		if(find_way_points(m_waypoints,get_coord(),coord_birth,true)){
			set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_PATROL),"");
			return boost::lexical_cast<std::uint64_t>(ai_data->params2)*1000;
		}else{
			LOG_EMPERY_DUNGEON_WARNING("find the way point to patrol src fail，dungeon_uuid = ",get_dungeon_uuid());
			set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_SEARCH_TARGET),"");
		}
	}else{
		set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_MONSTER_SEARCH_TARGET),"");
	}
	return boost::lexical_cast<std::uint64_t>(ai_data->params)*1000;
}

double DungeonObject::get_total_defense(){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon object data");
		return 0;
	}
	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon");
		return 0;
	}
	double total_defense = dungeon_object_type_data->defence * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_BONUS) / 1000.0 + dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_DEFENSE_BONUS) / 1000.0) +
	get_attribute(EmperyCenter::AttributeIds::ID_DEFENSE_ADD) / 1000.0 +
	dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_DEFENSE_ADD) / 1000.0;
	return total_defense;
}
double  DungeonObject::get_total_attack(){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon object data");
		return 0;
	}
	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon");
		return 0;
	}
	double total_attack = dungeon_object_type_data->attack * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0 + dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_ATTACK_BONUS) / 1000.0 ) +
	get_attribute(EmperyCenter::AttributeIds::ID_ATTACK_ADD) / 1000.0 +
	dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_ATTACK_ADD) / 1000.0;
	//狂暴buff
	double rage = dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_RAGE) / 1000.0;
	if(rage > 0.0){
		LOG_EMPERY_DUNGEON_FATAL("RAGE BUFF, RAGE = ",rage," before total_attack = ",total_attack);
		total_attack  = total_attack * (1.0 + rage/100.0);
		LOG_EMPERY_DUNGEON_FATAL("REGE BUFF end,total_attack = ",total_attack);
	}
	return total_attack;
}

double  DungeonObject::get_move_speed(){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon object data");
		return 0;
	}
	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon");
		return 0;
	}
	auto speed = dungeon_object_type_data->speed * (1.0 + get_attribute(EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0 + dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_SPEED_BONUS) / 1000.0) +
	get_attribute(EmperyCenter::AttributeIds::ID_SPEED_ADD) / 1000.0 +
	dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_SPEED_ADD) / 1000.0;
	return speed;
}

std::uint64_t  DungeonObject::get_attack_delay(){
	PROFILE_ME;
	const auto stand_by_interval = get_config<std::uint64_t>("stand_by_interval", 1000);

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon object data");
		return stand_by_interval;
	}
	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(!dungeon){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon");
		return stand_by_interval;
	}
	double attack_rate = dungeon_object_type_data->attack_speed + get_attribute(EmperyCenter::AttributeIds::ID_RATE_OF_FIRE_ADD) / 1000.0 +
	dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_RATE_OF_FIRE_ADD) / 1000.0;

	if(attack_rate < 0.0001 && attack_rate > -0.0001){
		return stand_by_interval;
	}else{
		std::uint64_t attack_delay = static_cast<std::uint64_t>(1000.0 / attack_rate);
		return attack_delay;
	}
}

bool          DungeonObject::can_use_skill(DungeonMonsterSkillId &skill_id,std::uint64_t now){
	PROFILE_ME;

	for(auto it = m_skills.begin(); it != m_skills.end(); ++it){
		auto &skill = it->second;
		//技能能量球仅在死亡时触发
		if(skill->get_skill_id() == ID_SKILL_ENERGY_SPHERE){
			continue;
		}
		if(skill->get_next_execute_time() < now){
			skill_id = skill->get_skill_id();
			LOG_EMPERY_DUNGEON_FATAL("MONSTER CAN USE SKILL,skill_id = ",skill_id);
			return true;
		}
	}
	return false;
}
std::uint64_t DungeonObject::use_skill(DungeonMonsterSkillId skill_id,std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	Coord coord;
	DungeonObjectUuid target_uuid;
	auto choice_a_target = choice_skill_target(skill_id,coord,target_uuid);
	if(choice_a_target){
		std::string action_param = get_action_param();
		set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_SKILL_SING),action_param);
		set_current_skill(skill_id,coord,target_uuid.str());
		auto skill_data = Data::Skill::require(skill_id);
		auto sing_delay = static_cast<std::uint64_t>(skill_data->sing_time * 1000);
		if(dungeon_client){
			if(sing_delay == 0){
				LOG_EMPERY_DUNGEON_WARNING("skill id ",skill_id.get(), " sing delay 0");
			}else{
				try{
					Msg::DS_DungeonObjectSkillSingAction msg;
					msg.dungeon_uuid = get_dungeon_uuid().str();
					msg.attacking_account_uuid   = get_owner_uuid().str();
					msg.attacking_object_uuid    = get_dungeon_object_uuid().str();
					msg.attacking_object_type_id = get_dungeon_object_type_id().get();
					msg.attacking_coord_x        = get_coord().x();
					msg.attacking_coord_y        = get_coord().y();
					msg.attacked_coord_x         = coord.x();
					msg.attacked_coord_y         = coord.y();
					msg.skill_type_id            = skill_id.get();
					msg.sing_delay               = sing_delay;
					auto sresult = dungeon_client->send_and_wait(msg);
					if(sresult.first != Msg::ST_OK){
						LOG_EMPERY_DUNGEON_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
						result = std::move(sresult);
						return do_finish_skill(skill_id,now);
					}
				} catch(std::exception &e){
					LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
					dungeon_client->shutdown(e.what());
				}
			}
		}
		return sing_delay;
	}
	//失败则直接取消
	auto delay = do_finish_skill(skill_id,now);
	return delay;
}

std::uint64_t DungeonObject::on_skill_singing_finish(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	LOG_EMPERY_DUNGEON_FATAL("MONSTER USE SKILL SING FINISH");
	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::require(dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	std::string action_param = get_action_param();
	set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_SKILL_CAST),action_param);
	auto skill_id = get_current_skill_id();
	auto coord    = get_skill_target_coord();
	auto skill_data = Data::Skill::require(skill_id);
	auto cast_delay = static_cast<std::uint64_t>(skill_data->cast_time * 1000);
	if(dungeon_client){
		if(cast_delay == 0){
			LOG_EMPERY_DUNGEON_WARNING("skill id ",skill_id.get(), " cast delay 0");
		}else{
			try{
				Msg::DS_DungeonObjectSkillCastAction msg;
				msg.dungeon_uuid = get_dungeon_uuid().str();
				msg.attacking_account_uuid   = get_owner_uuid().str();
				msg.attacking_object_uuid    = get_dungeon_object_uuid().str();
				msg.attacking_object_type_id = get_dungeon_object_type_id().get();
				msg.attacking_coord_x        = get_coord().x();
				msg.attacking_coord_y        = get_coord().y();
				msg.attacked_coord_x         = coord.x();
				msg.attacked_coord_y         = coord.y();
				msg.skill_type_id            = skill_id.get();
				msg.cast_delay               = cast_delay;
				auto sresult = dungeon_client->send_and_wait(msg);
				if(sresult.first != Msg::ST_OK){
					LOG_EMPERY_DUNGEON_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
					result = std::move(sresult);
					return do_finish_skill(skill_id,now);
				}
			} catch(std::exception &e){
				LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
				dungeon_client->shutdown(e.what());
			}
		}
	}
	return cast_delay;
}

std::uint64_t DungeonObject::on_skilling_casting_finish(std::pair<long, std::string> &result, std::uint64_t now){
	PROFILE_ME;

	LOG_EMPERY_DUNGEON_FATAL("MONSTER USE SKILL CAST FINISH");
	auto skill_id = get_current_skill_id();
	//执行各个技能的效果
	do_skill_effects(skill_id);
	auto delay = do_finish_skill(skill_id,now);
	return delay;
}

void          DungeonObject::check_current_skill(std::uint64_t now){
	PROFILE_ME;

	const auto dungeon_object_type_data = get_dungeon_object_type_data();
	if(!dungeon_object_type_data){
		LOG_EMPERY_DUNGEON_WARNING("NO dungeon object data");
		return;
	}
	const auto hp_total   = checked_mul(dungeon_object_type_data->max_soldier_count, dungeon_object_type_data->hp);
	const auto current_hp = static_cast<std::uint64_t>(get_attribute(EmperyCenter::AttributeIds::ID_HP_TOTAL));
	std::uint32_t percent = current_hp * 100 / hp_total;
	std::vector<DungeonMonsterSkillId> skill_candidates;
	for(unsigned i = 0; i < dungeon_object_type_data->skill_trigger.size(); ++i){
		auto percent_pair = dungeon_object_type_data->skill_trigger.at(i);
		if(percent_pair.first >= percent && percent_pair.second <= percent){
			skill_candidates = dungeon_object_type_data->skills.at(i);
			break;
		}
	}
	//删除不符合的技能
	auto it = m_skills.begin();
	while(it != m_skills.end()) {
		const auto skill_id = it->first;
		const auto cit = std::find(skill_candidates.begin(), skill_candidates.end(), skill_id);
		if (cit == skill_candidates.end()) {
			LOG_EMPERY_DUNGEON_FATAL("REMOVE NOT IN HP SKILL,SKILL_ID:",skill_id);
			it = m_skills.erase(it);
		}else{
			it++;
		}
	}
	//增加新增的技能
	for (auto it = skill_candidates.begin();  it != skill_candidates.end(); ++it){
		const auto skill_id = *it;
		auto sit = m_skills.find(skill_id);
		if(sit == m_skills.end()){
			auto skill = create_skill(skill_id);
			if(skill){
				auto next_skill_time = calculate_next_skill_time(skill_id,now);
				skill->set_next_execute_time(next_skill_time);
				LOG_EMPERY_DUNGEON_FATAL("this is a before a common attack ,and check inset new skill,skill_id = ",skill_id," execute_time = ",next_skill_time, " current_percent = ",percent);
				m_skills.emplace(skill_id,std::move(skill));
			}else{
				LOG_EMPERY_DUNGEON_ERROR("create skill fail, skill_id = ",skill_id);
			}
		}
	}
}

std::uint64_t  DungeonObject::calculate_next_skill_time(DungeonMonsterSkillId skill_id, std::uint64_t now){
	PROFILE_ME;

	auto skill_data = Data::Skill::require(skill_id);
	auto low = skill_data->cast_interval.first;
	auto high = skill_data->cast_interval.second;
	auto delay = Poseidon::rand32() % (saturated_sub(high, low) + 1) + low;
	return now + delay * 1000;
}

bool          DungeonObject::choice_skill_target(DungeonMonsterSkillId skill_id,Coord &coord,DungeonObjectUuid &dungeon_object_uuid){
	PROFILE_ME;

	auto skill_data = Data::Skill::require(skill_id);
	//施法对象是自己的返回
	if(skill_id == ID_SKILL_REFLEX_INJURY ||
		skill_id == ID_SKILL_RAGE){
			coord = get_coord();
			dungeon_object_uuid = get_dungeon_object_uuid();
		return true;
	}
	//不需要施法对象的直接返回
	if(skill_data->cast_object == SKILL_TARGET_NONE){
		coord = Coord(0,0);
		return true;
	}
	auto cast_range = skill_data->cast_range;
	std::vector<Coord> surrounding;
	for(unsigned i = 1; i <= cast_range; ++i){
		get_surrounding_coords(surrounding,get_coord(), i);
	}
	std::random_shuffle(surrounding.begin(),surrounding.end());
	if(surrounding.empty()){
		return false;
	}
	const auto dungeon = DungeonMap::require(m_dungeon_uuid);
	for(auto it = surrounding.begin(); it != surrounding.end(); ++it){
		auto &temp_coord = *it;
		auto pass_able = get_dungeon_coord_passable(dungeon->get_dungeon_type_id(),temp_coord);
		if(!pass_able){
			continue;
		}
		if(skill_data->cast_object == SKILL_TARGET_GRID){
			coord = temp_coord;
			return true;
		}
		auto target_object = dungeon->get_object(temp_coord);
		if(!target_object){
			continue;
		}
		if(skill_data->cast_object == SKILL_TARGET_FRIEND){
			if(target_object->get_owner_uuid() == get_owner_uuid()){
				coord = target_object->get_coord();
				dungeon_object_uuid = target_object->get_dungeon_object_uuid();
				return true;
			}
		}else if(skill_data->cast_object == SKILL_TARGET_ENEMY){
			if(target_object->get_owner_uuid() != get_owner_uuid()){
				dungeon_object_uuid = target_object->get_dungeon_object_uuid();
				coord = target_object->get_coord();
				return true;
			}
		}
	}
	return false;
}

std::uint64_t  DungeonObject::do_finish_skill(DungeonMonsterSkillId skill_id,std::uint64_t now){
	PROFILE_ME;

	auto it = m_skills.find(skill_id);
	if(it != m_skills.end()){
		auto &skill = it->second;
		auto next_skill_time = calculate_next_skill_time(skill_id,now);
		LOG_EMPERY_DUNGEON_FATAL("reset next skill time,skill_id = ",skill_id, " next_execute_time = ",next_skill_time);
		skill->set_next_execute_time(next_skill_time);
	}
	set_current_skill(DungeonMonsterSkillId(),Coord(0,0),"");
	std::string action_param = get_action_param();
	set_action(get_coord(), m_waypoints, static_cast<DungeonObject::Action>(ACT_ATTACK),action_param);
	std::uint64_t attack_delay = get_attack_delay();
	return attack_delay;
}

void           DungeonObject::do_skill_effects(DungeonMonsterSkillId skill_id){
	PROFILE_ME;

	auto it = m_skills.find(skill_id);
	if(it != m_skills.end()){
		auto &skill = it->second;
		skill->do_effects();
	}
}

boost::shared_ptr<Skill> DungeonObject::create_skill(DungeonMonsterSkillId skill_id){
	PROFILE_ME;

	boost::shared_ptr<Skill> skill = {};
	switch(skill_id.get()){
		case ID_SKILL_CLEAVE.get():
			skill = boost::make_shared<SkillCleave>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_CYCLONE.get():
			skill = boost::make_shared<SkillCyclone>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_FIRE_BRAND.get():
			skill = boost::make_shared<SkillFireBrand>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_CORROSION.get():
			skill = boost::make_shared<SkillCorrosion>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_REFLEX_INJURY.get():
			skill = boost::make_shared<SkillReflexInjury>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_RAGE.get():
			skill = boost::make_shared<SkillRage>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_SUMMON_SKULLS.get():
			skill = boost::make_shared<SkillSummonSkulls>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_SUMMON_MONSTER.get():
			skill = boost::make_shared<SkillSummonMonster>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_SOUL_ATTACK.get():
			skill = boost::make_shared<SkillSoulAttack>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		case ID_SKILL_ENERGY_SPHERE.get():
			skill = boost::make_shared<SkillEnergySphere>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
		default:
			LOG_EMPERY_DUNGEON_ERROR("unknown skill id = ",skill_id);
			skill = boost::make_shared<Skill>(skill_id,virtual_weak_from_this<DungeonObject>());
			break;
	}
	return skill;
}

bool           DungeonObject::can_reflex_injury(){
	PROFILE_ME;

	const auto dungeon = DungeonMap::require(m_dungeon_uuid);
	double reflex_injury = dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_REFLEX_INJURY) / 1000.0;
	if(reflex_injury > 0.0){
		return true;
	}
	return false;
}
void         DungeonObject::do_reflex_injury(std::uint64_t total_damage,boost::shared_ptr<DungeonObject> attacker){
	PROFILE_ME;
	if(!attacker){
		LOG_EMPERY_DUNGEON_WARNING("reflex injury attacker is null ?");
		return;
	}
	const auto dungeon = DungeonMap::require(m_dungeon_uuid);
	const auto dungeon_client = dungeon->get_dungeon_client();
	if(!dungeon_client){
		return;
	}
	double reflex_injury = dungeon->get_dungeon_attribute(get_dungeon_object_uuid(),get_owner_uuid(),EmperyCenter::AttributeIds::ID_REFLEX_INJURY) / 1000.0;
	if(reflex_injury <= 0.0){
		return;
	}
	double k = 0.06;
	double total_attack = total_damage * reflex_injury/100.0;
	double total_defense = attacker->get_total_defense();
	double relative_rate = Data::DungeonObjectRelative::get_relative(attacker->get_arm_attack_type(),attacker->get_arm_defence_type());
	std::uint64_t damage = total_attack * relative_rate * (1 - k *total_defense/(1 + k*total_defense));
	try{
		Msg::DS_DungeonObjectAttackAction msg;
		msg.dungeon_uuid = get_dungeon_uuid().str();
		msg.attacking_account_uuid = get_owner_uuid().str();
		msg.attacking_object_uuid = get_dungeon_object_uuid().str();
		msg.attacking_object_type_id = get_dungeon_object_type_id().get();
		msg.attacking_coord_x = get_coord().x();
		msg.attacking_coord_y = get_coord().y();
		msg.attacked_account_uuid = attacker->get_owner_uuid().str();
		msg.attacked_object_uuid = attacker->get_dungeon_object_uuid().str();
		msg.attacked_object_type_id = attacker->get_dungeon_object_type_id().get();
		msg.attacked_coord_x = attacker->get_coord().x();
		msg.attacked_coord_y = attacker->get_coord().y();
		msg.result_type = IMPACT_NORMAL;
		msg.soldiers_damaged = damage;
		auto sresult = dungeon_client->send_and_wait(msg);
		if(sresult.first != Msg::ST_OK){
			LOG_EMPERY_DUNGEON_DEBUG("Center server returned an error: code = ", sresult.first, ", msg = ", sresult.second);
			return ;
		}
		LOG_EMPERY_DUNGEON_ERROR("do reflex injury total_damage = ",total_damage, " reflex_injury = ",reflex_injury);
	} catch(std::exception &e){
		LOG_EMPERY_DUNGEON_WARNING("std::exception thrown: what = ", e.what());
		dungeon_client->shutdown(e.what());
	}
}

void           DungeonObject::do_die_skill(){
	PROFILE_ME;

	auto it = m_skills.find(ID_SKILL_ENERGY_SPHERE);
	if(it != m_skills.end()){
		auto &skill = it->second;
		skill->do_effects();
	}
}
}
