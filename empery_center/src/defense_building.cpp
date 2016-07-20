#include "precompiled.hpp"
#include "defense_building.hpp"
#include "mysql/defense_building.hpp"
#include "msg/sc_map.hpp"
#include "singletons/world_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/map_defense.hpp"
#include "attribute_ids.hpp"
#include "map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
	boost::shared_ptr<MySql::Center_DefenseBuilding> create_default_defense_obj(MapObjectUuid map_object_uuid){
		PROFILE_ME;

		auto obj = boost::make_shared<MySql::Center_DefenseBuilding>(map_object_uuid.get(),
			0, DefenseBuilding::MIS_NONE, 0, 0, 0, Poseidon::Uuid(), 0);
		obj->async_save(true, true);
		return obj;
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

		case DefenseBuilding::MIS_GARRISON:
			break;

		case DefenseBuilding::MIS_EVICT:
			obj->set_garrisoning_object_uuid({ });
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
	: MapObject(map_object_uuid, map_object_type_id, owner_uuid, parent_object_uuid, std::move(name), coord, created_time, false)
	, m_defense_obj(create_default_defense_obj(map_object_uuid))
{
}
DefenseBuilding::DefenseBuilding(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs,
	const std::vector<boost::shared_ptr<MySql::Center_DefenseBuilding>> &defense_objs)
	: MapObject(std::move(obj), attributes, buffs)
	, m_defense_obj(defense_objs.empty() ? create_default_defense_obj(MapObject::get_map_object_uuid())
	                                     : std::move(defense_objs.front()))
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

	self_heal();

	const auto utc_now = Poseidon::get_utc_time();

	if(check_defense_building_mission(m_defense_obj, utc_now)){
		recalculate_attributes(false);
	}
}
void DefenseBuilding::recalculate_attributes(bool recursive){
	PROFILE_ME;

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
	}

	const auto garrisoning_object_uuid = get_garrisoning_object_uuid();
	if(garrisoning_object_uuid){
		const auto garrisoning_object = WorldMap::get_map_object(garrisoning_object_uuid);
		if(!garrisoning_object){
			goto _bunker_done;
		}
		if(!garrisoning_object->is_garrisoned()){
			goto _bunker_done;
		}
		const auto garrisoning_object_type_id = garrisoning_object->get_map_object_type_id();

		modifiers[AttributeIds::ID_GARRISONING_BATTALION_TYPE_ID] = garrisoning_object_type_id.get();

		garrisoning_object->recalculate_attributes(false);

		for(auto it = COMBAT_ATTRIBUTES.begin(); it != COMBAT_ATTRIBUTES.end(); ++it){
			const auto attribute_id = *it;
			auto &value = modifiers[attribute_id];
			value += garrisoning_object->get_attribute(attribute_id);
		}
	}
_bunker_done:
	;

	set_attributes(std::move(modifiers));

	MapObject::recalculate_attributes(recursive);
}

bool DefenseBuilding::is_protectable() const {
	return true;
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

void DefenseBuilding::create_mission(DefenseBuilding::Mission mission, std::uint64_t duration, MapObjectUuid garrisoning_object_uuid){
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
	obj->set_garrisoning_object_uuid(garrisoning_object_uuid.get());

	if(check_defense_building_mission(obj, utc_now)){
		recalculate_attributes(false);
	}

	WorldMap::update_map_object(virtual_shared_from_this<DefenseBuilding>(), false);
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

//	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(0);
	obj->set_mission_time_end(0);
/*
	if(check_defense_building_mission(obj, utc_now)){
		recalculate_attributes(false);
	}
*/
	WorldMap::update_map_object(virtual_shared_from_this<DefenseBuilding>(), false);
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
		recalculate_attributes(false);
	}

	WorldMap::update_map_object(virtual_shared_from_this<DefenseBuilding>(), false);
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
		recalculate_attributes(false);
	}

	WorldMap::update_map_object(virtual_shared_from_this<DefenseBuilding>(), false);
}

MapObjectUuid DefenseBuilding::get_garrisoning_object_uuid() const {
	return MapObjectUuid(m_defense_obj->get_garrisoning_object_uuid());
}

void DefenseBuilding::self_heal(){
	PROFILE_ME;

	const auto building_level = get_level();
	const auto map_object_type_id = get_map_object_type_id();
	const auto defense_building_data = Data::MapDefenseBuildingAbstract::require(map_object_type_id, building_level);
	const auto defense_combat_data = Data::MapDefenseCombat::require(defense_building_data->defense_combat_id);
	const auto hp_per_soldier = std::max<std::uint64_t>(defense_combat_data->hp_per_soldier, 1);
	const auto max_hp_total = checked_mul(defense_combat_data->soldiers_max, hp_per_soldier);
	if(max_hp_total <= 0){
		return;
	}

	LOG_EMPERY_CENTER_TRACE("Self heal: map_object_uuid = ", get_map_object_uuid(), ", map_object_type_id = ", map_object_type_id,
		", building_level = ", building_level, ", self_healing_rate = ", defense_combat_data->self_healing_rate);

	const auto old_self_healed_time = m_defense_obj->get_last_self_healed_time();
	const auto old_hp_total = static_cast<std::uint64_t>(get_attribute(AttributeIds::ID_HP_TOTAL));

	const auto utc_now = Poseidon::get_utc_time();

	double amount_healed;
	if(old_self_healed_time == 0){
		amount_healed = saturated_sub(max_hp_total, old_hp_total);
	} else {
		const auto self_healing_rate = defense_combat_data->self_healing_rate;
		const auto soldiers_healed_perminute = std::ceil(self_healing_rate * max_hp_total - 0.001);
		const auto self_healing_duration = saturated_sub(utc_now, old_self_healed_time);
		amount_healed = self_healing_duration * soldiers_healed_perminute / 60000.0 + m_self_healing_remainder;
	}
	const auto rounded_amount_healed = static_cast<std::uint64_t>(amount_healed);
	const auto new_hp_total = std::min<std::uint64_t>(saturated_add(old_hp_total, rounded_amount_healed), max_hp_total);
	if(new_hp_total > old_hp_total){
		const auto new_soldier_count = static_cast<std::uint64_t>(std::ceil(static_cast<double>(new_hp_total) / hp_per_soldier - 0.001));
		boost::container::flat_map<AttributeId, std::int64_t> modifiers;
		modifiers.reserve(16);
		modifiers[AttributeIds::ID_SOLDIER_COUNT_MAX] = static_cast<std::int64_t>(defense_combat_data->soldiers_max);
		modifiers[AttributeIds::ID_SOLDIER_COUNT]     = static_cast<std::int64_t>(new_soldier_count);
		modifiers[AttributeIds::ID_HP_TOTAL]          = static_cast<std::int64_t>(new_hp_total);
		set_attributes(std::move(modifiers));
	}
	m_self_healing_remainder = amount_healed - rounded_amount_healed;
	m_defense_obj->set_last_self_healed_time(utc_now);

	WorldMap::update_map_object(virtual_shared_from_this<DefenseBuilding>(), false);
}

void DefenseBuilding::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_MapDefenseBuilding msg;
	msg.map_object_uuid         = m_defense_obj->unlocked_get_map_object_uuid().to_string();
	msg.building_level          = m_defense_obj->get_building_level();
	msg.mission                 = m_defense_obj->get_mission();
	msg.mission_duration        = m_defense_obj->get_mission_duration();
	msg.mission_time_remaining  = saturated_sub(m_defense_obj->get_mission_time_end(), utc_now);
	msg.garrisoning_object_uuid = m_defense_obj->unlocked_get_garrisoning_object_uuid().to_string();
	session->send(msg);
}

}
