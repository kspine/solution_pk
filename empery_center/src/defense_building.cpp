#include "precompiled.hpp"
#include "defense_building.hpp"
#include "mysql/defense_building.hpp"
#include "msg/sc_map.hpp"
#include "singletons/world_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/map.hpp"
#include "attribute_ids.hpp"
#include "map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_defense_building_message(Msg::SC_MapDefenseBuilding &msg,
		const boost::shared_ptr<MySql::Center_DefenseBuilding> &obj, std::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid         = obj->unlocked_get_map_object_uuid().to_string();
		msg.building_level          = obj->get_building_level();
		msg.mission                 = obj->get_mission();
		msg.mission_duration        = obj->get_mission_duration();
		msg.mission_time_remaining  = saturated_sub(obj->get_mission_time_end(), utc_now);
		msg.garrisoning_object_uuid = obj->unlocked_get_garrisoning_object_uuid().to_string();
	}

	bool check_defense_building_mission(const boost::shared_ptr<MySql::Center_DefenseBuilding> &obj, std::uint64_t utc_now){
		PROFILE_ME;

		const auto mission = DefenseBuilding::Mission(obj->get_mission());
		if(mission == DefenseBuilding::MIS_NONE){
			return false;
		}
		const auto mission_time_end = obj->get_mission_time_end();
		if(utc_now < mission_time_end){
			return false;
		}

		LOG_EMPERY_CENTER_DEBUG("Defense building mission complete: map_object_uuid = ", obj->get_map_object_uuid(),
			", mission = ", (unsigned)mission);
		switch(mission){
			unsigned level;

		case DefenseBuilding::MIS_CONSTRUCT:
			obj->set_building_level(1);
			break;

		case DefenseBuilding::MIS_UPGRADE:
			level = obj->get_building_level();
			obj->set_building_level(level + 1);
			break;

		case DefenseBuilding::MIS_DESTRUCT:
			obj->set_building_level(0);
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown defense building mission: map_object_uuid = ", obj->get_map_object_uuid(),
				", mission = ", (unsigned)mission);
			DEBUG_THROW(Exception, sslit("Unknown defense building mission"));
		}

		obj->set_mission(DefenseBuilding::MIS_NONE);
		obj->set_mission_duration(0);
		obj->set_mission_time_begin(0);
		obj->set_mission_time_end(0);

		return true;
	}
}

DefenseBuilding::DefenseBuilding(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time)
	: MapObject(map_object_uuid, map_object_type_id, owner_uuid,
		parent_object_uuid, std::move(name), coord, created_time, false)
	, m_defense_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_DefenseBuilding>(map_object_uuid.get(),
				0, MIS_NONE, 0, 0, 0, Poseidon::Uuid(), created_time);
			obj->async_save(true);
			return obj;
		}())
{
}
DefenseBuilding::DefenseBuilding(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	boost::shared_ptr<MySql::Center_DefenseBuilding> defense_obj)
	: MapObject(std::move(obj), attributes)
	, m_defense_obj(std::move(defense_obj))
{
}
DefenseBuilding::~DefenseBuilding(){
}

void DefenseBuilding::synchronize_with_player_additional(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	MapObject::synchronize_with_player_additional(session);

	synchronize_with_player(session);
}

void DefenseBuilding::pump_status(){
	PROFILE_ME;

	MapObject::pump_status();

	const auto utc_now = Poseidon::get_utc_time();

	if(check_defense_building_mission(m_defense_obj, utc_now)){
		recalculate_attributes();
	}

	const auto building_level = get_level();
	if(building_level > 0){
		const auto map_object_type_id = get_map_object_type_id();
		const auto defense_building_data = Data::MapDefenseBuildingAbstract::require(map_object_type_id, building_level);
		const auto defense_combat_data = Data::MapDefenseCombat::require(defense_building_data->defense_combat_id);
		const auto old_soldier_count = static_cast<std::uint64_t>(get_attribute(AttributeIds::ID_SOLDIER_COUNT));
		const auto max_soldier_count = defense_combat_data->soldiers_max;
		if(old_soldier_count < max_soldier_count){
			const auto heal_rate = defense_combat_data->self_healing_rate;
			const auto duration = saturated_sub(utc_now, m_defense_obj->get_last_self_healed_time());
			auto count_to_add = heal_rate * duration / 60000.0 + 0.001;
			std::uint64_t new_soldier_count;
			if(count_to_add > max_soldier_count - old_soldier_count){
				new_soldier_count = max_soldier_count;
			} else {
				new_soldier_count = static_cast<std::uint64_t>(old_soldier_count + count_to_add);
			}
			LOG_EMPERY_CENTER_DEBUG("Self heal: map_object_uuid = ", get_map_object_uuid(), ", map_object_type_id = ", map_object_type_id,
				", building_level = ", building_level, ", heal_rate = ", heal_rate, ", duration = ", duration,
				", old_soldier_count = ", old_soldier_count, ", max_soldier_count = ", max_soldier_count, ", new_soldier_count = ", new_soldier_count);

			boost::container::flat_map<AttributeId, std::int64_t> modifiers;
			modifiers[AttributeIds::ID_SOLDIER_COUNT] = static_cast<std::int64_t>(new_soldier_count);
			set_attributes(std::move(modifiers));

			m_defense_obj->set_last_self_healed_time(utc_now);
		}
	}
}
void DefenseBuilding::recalculate_attributes(){
	PROFILE_ME;

	MapObject::recalculate_attributes();

//	const auto utc_now = Poseidon::get_utc_time();

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(32);

	const auto map_object_type_id = get_map_object_type_id();
	if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
		const auto building_level = get_level();
		auto &display_level = modifiers[AttributeIds::ID_BUILDING_LEVEL];
		if(display_level < building_level){
			display_level = building_level;
		}

		const auto mission = static_cast<unsigned>(get_mission());
		modifiers[AttributeIds::ID_DEFENSE_BUILDING_MISSION] = mission;

		boost::shared_ptr<MapObject> parent_object;
		const auto parent_object_uuid = get_parent_object_uuid();
		if(parent_object_uuid){
			parent_object = WorldMap::get_map_object(parent_object_uuid);
		}
		if(parent_object){
			auto defense_attack_bonus_value = parent_object->get_attribute(AttributeIds::ID_DEFENSE_BUILDING_ATTACK_BONUS);
			auto &attack_bonus_value = modifiers[AttributeIds::ID_ATTACK_BONUS];
			attack_bonus_value += defense_attack_bonus_value;

			auto defense_defense_bonus_value = parent_object->get_attribute(AttributeIds::ID_DEFENSE_BUILDING_DEFENSE_BONUS);
			auto &defense_bonus_value = modifiers[AttributeIds::ID_DEFENSE_BONUS];
			defense_bonus_value += defense_defense_bonus_value;
		}
	}

	set_attributes(std::move(modifiers));
}

unsigned DefenseBuilding::get_level() const {
	return m_defense_obj->get_building_level();
}
DefenseBuilding::Mission DefenseBuilding::get_mission() const {
	return static_cast<Mission>(m_defense_obj->get_mission());
}
std::uint64_t DefenseBuilding::get_mission_duration() const {
	return m_defense_obj->get_mission_duration();
}
std::uint64_t DefenseBuilding::get_mission_time_begin() const {
	return m_defense_obj->get_mission_time_begin();
}
std::uint64_t DefenseBuilding::get_mission_time_end() const {
	return m_defense_obj->get_mission_time_end();
}

void DefenseBuilding::create_mission(DefenseBuilding::Mission mission, std::uint64_t duration){
	PROFILE_ME;

	const auto &obj = m_defense_obj;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission != MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("Defense building mission conflict: map_object_uuid = ", get_map_object_uuid(),
			", map_object_type_id = ", get_map_object_type_id());
		DEBUG_THROW(Exception, sslit("Defense building mission conflict"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	if(mission == MIS_CONSTRUCT){
		obj->set_building_level(0);
	}
	obj->set_mission(mission);
	obj->set_mission_duration(duration);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(saturated_add(utc_now, duration));

	if(check_defense_building_mission(obj, utc_now)){
		recalculate_attributes();
	}

	const auto map_object = WorldMap::get_map_object(get_map_object_uuid());
	if(map_object){
		const auto session = PlayerSessionMap::get(map_object->get_owner_uuid());
		if(session){
			try {
				Msg::SC_MapDefenseBuilding msg;
				fill_defense_building_message(msg, obj, utc_now);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}
void DefenseBuilding::cancel_mission(){
	PROFILE_ME;

	const auto &obj = m_defense_obj;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No defense building mission: map_object_uuid = ", get_map_object_uuid(),
			", map_object_type_id = ", get_map_object_type_id());
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(0);
	obj->set_mission_time_end(0);
/*
	if(check_defense_building_mission(obj, utc_now)){
		recalculate_attributes();
	}
*/
	const auto map_object = WorldMap::get_map_object(get_map_object_uuid());
	if(map_object){
		const auto session = PlayerSessionMap::get(map_object->get_owner_uuid());
		if(session){
			try {
				Msg::SC_MapDefenseBuilding msg;
				fill_defense_building_message(msg, obj, utc_now);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}
void DefenseBuilding::speed_up_mission(std::uint64_t delta_duration){
	PROFILE_ME;

	const auto &obj = m_defense_obj;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No defense building mission: map_object_uuid = ", get_map_object_uuid(),
			", map_object_type_id = ", get_map_object_type_id());
		DEBUG_THROW(Exception, sslit("No defense building mission"));
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission_time_end(saturated_sub(obj->get_mission_time_end(), delta_duration));

	if(check_defense_building_mission(obj, utc_now)){
		recalculate_attributes();
	}

	const auto map_object = WorldMap::get_map_object(get_map_object_uuid());
	if(map_object){
		const auto session = PlayerSessionMap::get(map_object->get_owner_uuid());
		if(session){
			try {
				Msg::SC_MapDefenseBuilding msg;
				fill_defense_building_message(msg, obj, utc_now);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}
void DefenseBuilding::forced_replace_level(unsigned building_level){
	PROFILE_ME;

	const auto &obj = m_defense_obj;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No defense building mission: map_object_uuid = ", get_map_object_uuid(),
			", map_object_type_id = ", get_map_object_type_id());
		DEBUG_THROW(Exception, sslit("No defense building mission"));
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_building_level(building_level);
	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(0);
	obj->set_mission_time_end(0);

	if(check_defense_building_mission(obj, utc_now)){
		recalculate_attributes();
	}

	const auto map_object = WorldMap::get_map_object(get_map_object_uuid());
	if(map_object){
		const auto session = PlayerSessionMap::get(map_object->get_owner_uuid());
		if(session){
			try {
				Msg::SC_MapDefenseBuilding msg;
				fill_defense_building_message(msg, obj, utc_now);
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}

MapObjectUuid DefenseBuilding::get_garrisoning_object_uuid() const {
	return MapObjectUuid(m_defense_obj->get_garrisoning_object_uuid());
}
void DefenseBuilding::set_garrisoning_object_uuid(MapObjectUuid garrisoning_object_uuid){
	m_defense_obj->set_garrisoning_object_uuid(garrisoning_object_uuid.get());
}

void DefenseBuilding::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_MapDefenseBuilding msg;
	fill_defense_building_message(msg, m_defense_obj, utc_now);
	session->send(msg);
}

}
