#include "precompiled.hpp"
#include "warehouse_building.hpp"
#include "mysql/defense_building.hpp"
#include "msg/sc_map.hpp"
#include "singletons/world_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/map_object_type.hpp"
#include "attribute_ids.hpp"
#include "map_object_type_ids.hpp"
#include "data/map_defense.hpp"
#include "data/legion_building_config.hpp"
#include "buff_ids.hpp"
#include "map_object.hpp"
#include "msg/sc_legion_building.hpp"
#include "singletons/legion_building_map.hpp"
#include "data/global.hpp"
#include "mysql/map_object.hpp"

namespace EmperyCenter {

namespace {
	boost::shared_ptr<MySql::Center_WarehouseBuilding> create_default_warehouse_obj(MapObjectUuid map_object_uuid,LegionUuid legion_uuid){
		PROFILE_ME;

		auto obj = boost::make_shared<MySql::Center_WarehouseBuilding>(map_object_uuid.get(),legion_uuid.get(),
			0, WarehouseBuilding::MIS_NONE, 0, 0, 0, Poseidon::Uuid(), 0,0,0,0,0);
		obj->async_save(true, true);
		return obj;
	}

	bool check_defense_building_mission(const boost::shared_ptr<MySql::Center_WarehouseBuilding> &obj, std::uint64_t utc_now){
		PROFILE_ME;

	//	LOG_EMPERY_CENTER_ERROR("WarehouseBuilding check_defense_building_mission*******************",utc_now);

		const auto mission = WarehouseBuilding::Mission(obj->get_mission());
		if(mission == WarehouseBuilding::MIS_NONE){
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

		case WarehouseBuilding::MIS_CONSTRUCT:
			obj->set_building_level(1);
			break;

		case WarehouseBuilding::MIS_UPGRADE:
			{
				level = obj->get_building_level();
				obj->set_building_level(level + 1);

				const auto warehost_object = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(MapObjectUuid(obj->get_map_object_uuid())));
				if(warehost_object)
				{
					// 需要计算血量问题
					const auto oldbuildinginfo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(level));
					if(!oldbuildinginfo)
						break;

					const auto buildinginfo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(level + 1));
					if(!buildinginfo)
						break;

					const auto old_combat_data = Data::MapDefenseCombat::require(DefenseCombatId(oldbuildinginfo->building_combat_attributes));
					if(!old_combat_data)
						break;
					const auto old_hp_max =boost::lexical_cast<std::uint64_t>(old_combat_data->hp_per_soldier);


					const auto cur_combat_data = Data::MapDefenseCombat::require(DefenseCombatId(buildinginfo->building_combat_attributes));
					if(!cur_combat_data)
						break;

					const auto cur_hp_max =boost::lexical_cast<std::uint64_t>(cur_combat_data->hp_per_soldier);

					const auto curhp = boost::lexical_cast<std::uint64_t>(warehost_object->get_attribute(AttributeIds::ID_HP_TOTAL));

					const auto changevalue = static_cast<std::int64_t>(curhp + cur_hp_max - old_hp_max);

					warehost_object->set_maxhp(cur_hp_max);

					boost::container::flat_map<AttributeId, std::int64_t> modifiers;

					modifiers[AttributeIds::ID_HP_TOTAL] = changevalue;

					warehost_object->set_attributes(modifiers);

					warehost_object->on_hp_change(changevalue);

				}
			}

			break;

		case WarehouseBuilding::MIS_DESTRUCT:
			obj->set_building_level(0);
			break;

		case WarehouseBuilding::MIS_GARRISON:
			break;

		case WarehouseBuilding::MIS_EVICT:
			obj->set_garrisoning_object_uuid({ });
			break;

		case WarehouseBuilding::MIS_OPENCD:
			obj->set_cd_time(0);
			break;

		case WarehouseBuilding::MIS_OPENEFFECT:
			{
				obj->set_output_type(0);
				obj->set_output_amount(0);
				obj->set_effect_time(0);
				break;
			}


		default:
			LOG_EMPERY_CENTER_ERROR("Unknown defense building mission: map_object_uuid = ", obj->get_map_object_uuid(),
				", mission = ", (unsigned)mission);
			DEBUG_THROW(Exception, sslit("Unknown defense building mission"));
		}

		obj->set_mission(WarehouseBuilding::MIS_NONE);
		obj->set_mission_duration(0);
		obj->set_mission_time_begin(0);
		obj->set_mission_time_end(0);

		return true;
	}
}

WarehouseBuilding::WarehouseBuilding(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid,
	MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time,LegionUuid legion_uuid)
	: MapObject(map_object_uuid, map_object_type_id, owner_uuid, parent_object_uuid, std::move(name), coord, created_time, UINT64_MAX, false)
	, m_defense_obj(create_default_warehouse_obj(map_object_uuid,legion_uuid))
{
}
WarehouseBuilding::WarehouseBuilding(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs,
	const std::vector<boost::shared_ptr<MySql::Center_WarehouseBuilding>> &defense_objs)
	: MapObject(std::move(obj), attributes, buffs)
	, m_defense_obj(std::move(defense_objs.front()))
{
//	LOG_EMPERY_CENTER_ERROR("WarehouseBuilding 构造函数 attributes size*******************",attributes.size(), " buffs size:",buffs.size()," defense_objs size:",defense_objs.size());
	for(auto it = buffs.begin(); it != buffs.end(); ++it)
	{
		auto &buff = *it; 
		auto buffid = BuffId(buff->get_buff_id());
		if(buffid == BuffIds::ID_LEGION_WAREHOUSE_BUFF1 || buffid == BuffIds::ID_LEGION_WAREHOUSE_BUFF2 || buffid == BuffIds::ID_LEGION_WAREHOUSE_BUFF3)
		{
			m_buffid = boost::lexical_cast<unsigned int>(buffid);
			break;
		}
	}
}
WarehouseBuilding::~WarehouseBuilding(){
}

void WarehouseBuilding::synchronize_with_player_additional(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	MapObject::synchronize_with_player_additional(session);

	synchronize_with_player(session);
}

void WarehouseBuilding::pump_status(){
	PROFILE_ME;

	MapObject::pump_status();

	self_heal();

	const auto utc_now = Poseidon::get_utc_time();

	if(check_defense_building_mission(m_defense_obj, utc_now)){
		recalculate_attributes(false);
	}
}
void WarehouseBuilding::recalculate_attributes(bool recursive){
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

bool WarehouseBuilding::is_protectable() const {
	return true;
}

LegionUuid WarehouseBuilding::get_legion_uuid() const
{
	return LegionUuid(m_defense_obj->get_legion_uuid());
}

unsigned WarehouseBuilding::get_level() const {
	PROFILE_ME;
	return m_defense_obj->get_building_level();
}
WarehouseBuilding::Mission WarehouseBuilding::get_mission() const {
	PROFILE_ME;
	return static_cast<Mission>(m_defense_obj->get_mission());
}
std::uint64_t WarehouseBuilding::get_mission_duration() const {
	PROFILE_ME;
	return m_defense_obj->get_mission_duration();
}
std::uint64_t WarehouseBuilding::get_mission_time_begin() const {
	PROFILE_ME;
	return m_defense_obj->get_mission_time_begin();
}
std::uint64_t WarehouseBuilding::get_mission_time_end() const {
	PROFILE_ME;
	return m_defense_obj->get_mission_time_end();
}

std::uint64_t WarehouseBuilding::get_cd_time()
{
	PROFILE_ME;
	return m_defense_obj->get_cd_time();
}

std::int64_t WarehouseBuilding::get_max_hp()
{
	PROFILE_ME;
	return m_maxHp;
}

std::uint64_t WarehouseBuilding::get_output_amount()
{
	PROFILE_ME;
	return m_defense_obj->get_output_amount();
}

std::uint64_t WarehouseBuilding::get_output_type()
{
	PROFILE_ME;
	return m_defense_obj->get_output_type();
}

std::uint64_t WarehouseBuilding::get_effect_time()
{
	PROFILE_ME;
	return m_defense_obj->get_effect_time();
}

void WarehouseBuilding::set_output_amount(std::uint64_t amount)
{
	PROFILE_ME;

	m_defense_obj->set_output_amount(amount);
	// 采集完了,把采集内容清空了
	if(amount == 0)
	{
		m_defense_obj->set_output_type( 0 );
		if(WarehouseBuilding::Mission::MIS_OPENEFFECT  == get_mission())
			cancel_mission();
	}
}

void WarehouseBuilding::create_mission(WarehouseBuilding::Mission mission, std::uint64_t duration, MapObjectUuid garrisoning_object_uuid){
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

	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);
}
void WarehouseBuilding::cancel_mission(){
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
	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);
}
void WarehouseBuilding::speed_up_mission(std::uint64_t delta_duration){
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

	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);
}
void WarehouseBuilding::forced_replace_level(unsigned building_level){
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

	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);
}

MapObjectUuid WarehouseBuilding::get_garrisoning_object_uuid() const {
	return MapObjectUuid(m_defense_obj->get_garrisoning_object_uuid());
}

void WarehouseBuilding::self_heal(){
	PROFILE_ME;

	const auto building_level = get_level();
	const auto map_object_type_id = get_map_object_type_id();
	const auto buildinginfo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(get_level()));
	if(!buildinginfo)
		return;

	const auto defense_combat_data = Data::MapDefenseCombat::require(DefenseCombatId(buildinginfo->building_combat_attributes));
	const auto hp_per_soldier = std::max<std::uint64_t>(defense_combat_data->hp_per_soldier, 1);
	const auto max_hp_total = checked_mul(defense_combat_data->soldiers_max, hp_per_soldier);
	if(max_hp_total <= 0){
		return;
	}

	m_maxHp = boost::lexical_cast<std::int64_t>(max_hp_total);

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

	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);
}

void WarehouseBuilding::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto legionbuilding = LegionBuildingMap::find_by_map_object_uuid(get_map_object_uuid());
	if(legionbuilding)
	{
		Msg::SC_LegionBuildings msg;
		auto &elem = *msg.buildings.emplace(msg.buildings.end());
		elem.legion_building_uuid = LegionBuildingUuid(legionbuilding->get_legion_building_uuid()).str();
		elem.legion_uuid = LegionUuid(legionbuilding->get_legion_uuid()).str();
		elem.map_object_uuid = boost::lexical_cast<std::string>(get_map_object_uuid());
		const auto& mapobject = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(MapObjectUuid(elem.map_object_uuid)));
		if(mapobject)
		{
			const auto utc_now = Poseidon::get_utc_time();
			elem.x                  	 = mapobject->get_coord().x();
			elem.y                  	 = mapobject->get_coord().y();
			elem.building_level          = boost::lexical_cast<std::uint64_t>(mapobject->get_attribute(AttributeIds::ID_BUILDING_LEVEL));
			elem.mission                 = mapobject->get_mission();
			elem.mission_duration        = mapobject->get_mission_duration();
			elem.mission_time_remaining  = saturated_sub(mapobject->get_mission_time_end(), utc_now);
			elem.output_type			 = mapobject->get_output_type();
			elem.output_amount			 = mapobject->get_output_amount();
			elem.cd_time_remaining		 = saturated_sub(mapobject->get_cd_time(), utc_now);
		}


	//	LOG_EMPERY_CENTER_WARNING("legion buildings count:",msg.buildings.size());

		session->send(msg);
	}

}

void WarehouseBuilding::on_hp_change(std::int64_t curhp)
{
	LOG_EMPERY_CENTER_ERROR("WarehouseBuilding on_attack: curhp = ",curhp);

	if(curhp == 0)
	{
		// 如果当前血量为0，则摧毁
		// delete_from_game();
	}
	else
	{
		// 否则，按照当前血量和最大血量来计算debuff
		const auto buildinginfo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(get_level()));
		if(buildinginfo)
		{
			// 取得最大血量
			const auto defense_combat_data = Data::MapDefenseCombat::require(DefenseCombatId(buildinginfo->building_combat_attributes));
			if(defense_combat_data)
			{
				const auto rate = boost::lexical_cast<std::uint64_t>(boost::lexical_cast<std::uint64_t>(curhp) * 100  / defense_combat_data->hp_per_soldier);
				LOG_EMPERY_CENTER_ERROR("WarehouseBuilding on_attack: rate = ",rate);
				unsigned int buffid = 0;
				for(auto it = buildinginfo->damage_buff.rbegin(); it != buildinginfo->damage_buff.rend(); ++it)
				{
					LOG_EMPERY_CENTER_ERROR("WarehouseBuilding on_attack: config rate = ",it->first);
					if(rate < it->first)
					{
						buffid = boost::lexical_cast<unsigned int>(it->second);
					}
				}

				LOG_EMPERY_CENTER_ERROR("WarehouseBuilding on_attack: buffid = ",buffid, " m_buffid:",m_buffid);
				if(buffid != 0)
				{
					if(m_buffid != buffid)
					{
						const auto buff = is_buff_in_effect(BuffId(m_buffid));
						if(buff)
						{
							LOG_EMPERY_CENTER_ERROR("之前存在buff = 先清空",m_buffid);
							clear_buff(BuffId(m_buffid));
						}

						const auto utc_now = Poseidon::get_utc_time();
						set_buff(BuffId(buffid), utc_now,get_effect_time());
						m_buffid = buffid;
					}
				}
			}
			else
			{
				LOG_EMPERY_CENTER_ERROR("WarehouseBuilding on_attack: 没找到属性 ");
			}
		}
	}

	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);
}

void WarehouseBuilding::open(unsigned output_type,unsigned amount,std::uint64_t effect_time,std::uint64_t  opencd_time)
{
	const auto &obj = m_defense_obj;
	obj->set_output_type( output_type );
	obj->set_output_amount(amount);
	obj->set_effect_time(effect_time);
	obj->set_cd_time(opencd_time);

//	create_mission(WarehouseBuilding::MIS_OPENCD, opencd_time, { } );
	create_mission(WarehouseBuilding::MIS_OPENEFFECT, effect_time, { } );
}

void WarehouseBuilding::set_maxhp(std::uint64_t maxhp)
{
	m_maxHp = boost::lexical_cast<std::int64_t>(maxhp);
}

std::uint64_t WarehouseBuilding::harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated){
	PROFILE_ME;

	const auto resource_id = get_output_type();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No resource id: resource_crate_uuid = ", get_output_type());
		return 0;
	}
	const auto amount_remaining = get_output_amount();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: resource_crate_uuid = ", get_output_amount());
		return 0;
	}

	// 判断debuff
	std::uint64_t rate = 0;
	if(is_buff_in_effect(BuffIds::ID_LEGION_WAREHOUSE_BUFF3))
	{
		// 100%
		rate = Data::Global::as_unsigned(Data::Global::SLOT_LEGION_WAREHOUSE_DEBUFF3);
	}
	else if(is_buff_in_effect(BuffIds::ID_LEGION_WAREHOUSE_BUFF2))
	{
		// 50%
		rate = Data::Global::as_unsigned(Data::Global::SLOT_LEGION_WAREHOUSE_DEBUFF2);
	}
	else if(is_buff_in_effect(BuffIds::ID_LEGION_WAREHOUSE_BUFF1))
	{
		// 20%
		rate = Data::Global::as_unsigned(Data::Global::SLOT_LEGION_WAREHOUSE_DEBUFF1);
	}

	if(rate > 100)
	{
		LOG_EMPERY_CENTER_DEBUG("have  debuff 100 rate");
		return 0;
	}


	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining) * (100 - rate) / 100;
	const auto amount_added =harvester->load_resource(ResourceId(resource_id), rounded_amount_removable, false, true);
	const auto amount_removed = saturated ? rounded_amount_removable : amount_added;

	LOG_EMPERY_CENTER_DEBUG("WarehouseBuilding harvest*******************",amount_to_harvest, "amount_remaining:",amount_remaining," amount_removed:" ,amount_removed," amount_added:",amount_added);

	set_output_amount(saturated_sub(amount_remaining, amount_removed));

	WorldMap::update_map_object(virtual_shared_from_this<WarehouseBuilding>(), false);

	return amount_removed;
}


}
