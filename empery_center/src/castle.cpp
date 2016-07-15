#include "precompiled.hpp"
#include "castle.hpp"
#include "flag_guard.hpp"
#include "transaction_element.hpp"
#include "map_object.hpp"
#include "map_object_type_ids.hpp"
#include "mysql/castle.hpp"
#include "msg/sc_castle.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/castle.hpp"
#include "resource_ids.hpp"
#include "events/castle.hpp"
#include "attribute_ids.hpp"
#include "map_cell.hpp"
#include "reason_ids.hpp"
#include "data/map_object_type.hpp"
#include "building_type_ids.hpp"
#include "account_utilities.hpp"
#include "singletons/world_map.hpp"
#include "buff_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_building_base_info(Castle::BuildingBaseInfo &info, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj){
		PROFILE_ME;

		info.building_base_id      = BuildingBaseId(obj->get_building_base_id());
		info.building_id           = BuildingId(obj->get_building_id());
		info.building_level        = obj->get_building_level();
		info.mission               = Castle::Mission(obj->get_mission());
		info.mission_duration      = obj->get_mission_duration();
		info.mission_time_begin    = obj->get_mission_time_begin();
		info.mission_time_end      = obj->get_mission_time_end();
	}
	void fill_tech_info(Castle::TechInfo &info, const boost::shared_ptr<MySql::Center_CastleTech> &obj){
		PROFILE_ME;

		info.tech_id               = TechId(obj->get_tech_id());
		info.tech_level            = obj->get_tech_level();
		info.mission               = Castle::Mission(obj->get_mission());
		info.mission_duration      = obj->get_mission_duration();
		info.mission_time_begin    = obj->get_mission_time_begin();
		info.mission_time_end      = obj->get_mission_time_end();
	}
	void fill_resource_info(Castle::ResourceInfo &info, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		info.resource_id           = ResourceId(obj->get_resource_id());
		info.amount                = obj->get_amount();
	}
	void fill_soldier_info(Castle::SoldierInfo &info, const boost::shared_ptr<MySql::Center_CastleBattalion> &obj){
		PROFILE_ME;

		info.map_object_type_id    = MapObjectTypeId(obj->get_map_object_type_id());
		info.count                 = obj->get_count();
		info.enabled               = obj->get_enabled();
	}
	void fill_soldier_production_info(Castle::SoldierProductionInfo &info, const boost::shared_ptr<MySql::Center_CastleBattalionProduction> &obj){
		PROFILE_ME;

		info.building_base_id      = BuildingBaseId(obj->get_building_base_id());
		info.map_object_type_id    = MapObjectTypeId(obj->get_map_object_type_id());
		info.count                 = obj->get_count();
		info.production_duration   = obj->get_production_duration();
		info.production_time_begin = obj->get_production_time_begin();
		info.production_time_end   = obj->get_production_time_end();
	}
	void fill_wounded_soldier_info(Castle::WoundedSoldierInfo &info, const boost::shared_ptr<MySql::Center_CastleWoundedSoldier> &obj){
		PROFILE_ME;

		info.map_object_type_id    = MapObjectTypeId(obj->get_map_object_type_id());
		info.count                 = obj->get_count();
	}
	void fill_treatment_info(Castle::TreatmentInfo &info, const boost::shared_ptr<MySql::Center_CastleTreatment> &obj){
		PROFILE_ME;

		info.map_object_type_id    = MapObjectTypeId(obj->get_map_object_type_id());
		info.count                 = obj->get_count();
		info.duration              = obj->get_duration();
		info.time_begin            = obj->get_time_begin();
		info.time_end              = obj->get_time_end();
	}

	void fill_building_message(Msg::SC_CastleBuildingBase &msg,
		const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj, std::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.building_base_id       = obj->get_building_base_id();
		msg.building_id            = obj->get_building_id();
		msg.building_level         = obj->get_building_level();
		msg.mission                = obj->get_mission();
		msg.mission_duration       = obj->get_mission_duration();
		// msg.reserved
		// msg.reserved2
		msg.mission_time_remaining = saturated_sub(obj->get_mission_time_end(), utc_now);
	}
	void fill_tech_message(Msg::SC_CastleTech &msg, const boost::shared_ptr<MySql::Center_CastleTech> &obj, std::uint64_t utc_now){
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.tech_id                = obj->get_tech_id();
		msg.tech_level             = obj->get_tech_level();
		msg.mission                = obj->get_mission();
		msg.mission_duration       = obj->get_mission_duration();
		// msg.reserved
		// msg.reserved2
		msg.mission_time_remaining = saturated_sub(obj->get_mission_time_end(), utc_now);
	}
	void fill_resource_message(Msg::SC_CastleResource &msg, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.resource_id            = obj->get_resource_id();
		msg.amount                 = obj->get_amount();
	}
	void fill_soldier_message(Msg::SC_CastleSoldier &msg, const boost::shared_ptr<MySql::Center_CastleBattalion> &obj){
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.map_object_type_id     = obj->get_map_object_type_id();
		msg.count                  = obj->get_count();
		msg.enabled                = obj->get_enabled();
	}
	void fill_soldier_production_message(Msg::SC_CastleSoldierProduction &msg,
		const boost::shared_ptr<MySql::Center_CastleBattalionProduction> &obj, std::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid           = obj->unlocked_get_map_object_uuid().to_string();
		msg.building_base_id          = obj->get_building_base_id();
		msg.map_object_type_id        = obj->get_map_object_type_id();
		msg.count                     = obj->get_count();
		msg.production_duration       = obj->get_production_duration();
		msg.production_time_remaining = saturated_sub(obj->get_production_time_end(), utc_now);
	}
	void fill_wounded_soldier_message(Msg::SC_CastleWoundedSoldier &msg, const boost::shared_ptr<MySql::Center_CastleWoundedSoldier> &obj){
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.map_object_type_id     = obj->get_map_object_type_id();
		msg.count                  = obj->get_count();
	}
	void fill_treatment_message(Msg::SC_CastleTreatment &msg, MapObjectUuid map_object_uuid,
		const boost::container::flat_map<MapObjectTypeId, boost::shared_ptr<MySql::Center_CastleTreatment>> &objs, std::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid        = map_object_uuid.str();
		msg.soldiers.reserve(objs.size());
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = it->second;
			auto &soldier = *msg.soldiers.emplace(msg.soldiers.end());
			soldier.map_object_type_id   = obj->get_map_object_type_id();
			soldier.count                = obj->get_count();
			// pick any
			msg.treatment_duration       = obj->get_duration();
			msg.treatment_time_remaining = saturated_sub(obj->get_time_end(), utc_now);
		}
	}

	bool check_building_mission(const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj, std::uint64_t utc_now){
		PROFILE_ME;

		const auto mission = Castle::Mission(obj->get_mission());
		if(mission == Castle::MIS_NONE){
			return false;
		}
		const auto mission_time_end = obj->get_mission_time_end();
		if(utc_now < mission_time_end){
			return false;
		}

		LOG_EMPERY_CENTER_DEBUG("Building mission complete: map_object_uuid = ", obj->get_map_object_uuid(),
			", building_base_id = ", obj->get_building_base_id(), ", building_id = ", obj->get_building_id(), ", mission = ", (unsigned)mission);
		switch(mission){
			unsigned level;

		case Castle::MIS_CONSTRUCT:
			obj->set_building_level(1);
			break;

		case Castle::MIS_UPGRADE:
			level = obj->get_building_level();
			obj->set_building_level(level + 1);
			break;

		case Castle::MIS_DESTRUCT:
			obj->set_building_id(0);
			obj->set_building_level(0);
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown building mission: map_object_uuid = ", obj->get_map_object_uuid(),
				", building_base_id = ", obj->get_building_base_id(), ", mission = ", (unsigned)mission);
			DEBUG_THROW(Exception, sslit("Unknown building mission"));
		}

		obj->set_mission(Castle::MIS_NONE);
		obj->set_mission_duration(0);
		obj->set_mission_time_begin(0);
		obj->set_mission_time_end(0);

		return true;
	}
	bool check_tech_mission(const boost::shared_ptr<MySql::Center_CastleTech> &obj, std::uint64_t utc_now){
		PROFILE_ME;

		const auto mission = Castle::Mission(obj->get_mission());
		if(mission == Castle::MIS_NONE){
			return false;
		}
		const auto mission_time_end = obj->get_mission_time_end();
		if(utc_now < mission_time_end){
			return false;
		}

		LOG_EMPERY_CENTER_DEBUG("Tech mission complete: map_object_uuid = ", obj->get_map_object_uuid(),
			", tech_id = ", obj->get_tech_id(), ", mission = ", (unsigned)mission);
		switch(mission){
			unsigned level;
/*
		case Castle::MIS_CONSTRUCT:
			obj->set_tech_level(1);
			break;
*/
		case Castle::MIS_UPGRADE:
			level = obj->get_tech_level();
			obj->set_tech_level(level + 1);
			break;
/*
		case Castle::MIS_DESTRUCT:
			obj->set_tech_id(0);
			obj->set_tech_level(0);
			break;
*/
		default:
			LOG_EMPERY_CENTER_ERROR("Unknown tech mission: map_object_uuid = ", obj->get_map_object_uuid(),
				", tech_id = ", obj->get_tech_id(), ", mission = ", (unsigned)mission);
			DEBUG_THROW(Exception, sslit("Unknown tech mission"));
		}

		obj->set_mission(Castle::MIS_NONE);
		obj->set_mission_duration(0);
		obj->set_mission_time_end(0);

		return true;
	}
	bool check_treatment_mission(const boost::shared_ptr<MySql::Center_CastleTreatment> &obj, std::uint64_t utc_now){
		PROFILE_ME;

		const auto time_end = obj->get_time_end();
		if(utc_now < time_end){
			return false;
		}

		LOG_EMPERY_CENTER_DEBUG("Treatment mission complete: map_object_uuid = ", obj->get_map_object_uuid(),
			", map_object_type_id = ", obj->get_map_object_type_id());

		obj->set_duration(0);
		obj->set_time_end(0);

		return true;
	}
}

Castle::Castle(MapObjectUuid map_object_uuid, AccountUuid owner_uuid, MapObjectUuid parent_object_uuid,
	std::string name, Coord coord, std::uint64_t created_time)
	: DefenseBuilding(map_object_uuid, MapObjectTypeIds::ID_CASTLE, owner_uuid, parent_object_uuid,
		std::move(name), coord, created_time)
{
}
Castle::Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs,
	const std::vector<boost::shared_ptr<MySql::Center_DefenseBuilding>> &defense_objs,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
	const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
	const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBattalion>> &soldiers,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBattalionProduction>> &soldier_production,
	const std::vector<boost::shared_ptr<MySql::Center_CastleWoundedSoldier>> &wounded_soldiers,
	const std::vector<boost::shared_ptr<MySql::Center_CastleTreatment>> &treatment)
	: DefenseBuilding(std::move(obj), attributes, buffs, defense_objs)
{
	for(auto it = buildings.begin(); it != buildings.end(); ++it){
		const auto &obj = *it;
		m_buildings.emplace(BuildingBaseId(obj->get_building_base_id()), obj);
	}
	for(auto it = techs.begin(); it != techs.end(); ++it){
		const auto &obj = *it;
		m_techs.emplace(TechId(obj->get_tech_id()), obj);
	}
	for(auto it = resources.begin(); it != resources.end(); ++it){
		const auto &obj = *it;
		m_resources.emplace(ResourceId(obj->get_resource_id()), obj);
	}
	for(auto it = soldiers.begin(); it != soldiers.end(); ++it){
		const auto &obj = *it;
		m_soldiers.emplace(MapObjectTypeId(obj->get_map_object_type_id()), obj);
	}
	for(auto it = soldier_production.begin(); it != soldier_production.end(); ++it){
		const auto &obj = *it;
		const auto building_base_id = BuildingBaseId(obj->get_building_base_id());
		if(!building_base_id){
			m_population_production_stamps = obj;
		} else {
			m_soldier_production.emplace(building_base_id, obj);
		}
	}
	for(auto it = wounded_soldiers.begin(); it != wounded_soldiers.end(); ++it){
		const auto &obj = *it;
		m_wounded_soldiers.emplace(MapObjectTypeId(obj->get_map_object_type_id()), obj);
	}
	for(auto it = treatment.begin(); it != treatment.end(); ++it){
		const auto &obj = *it;
		m_treatment.emplace(MapObjectTypeId(obj->get_map_object_type_id()), obj);
	}
}
Castle::~Castle(){
}

void Castle::pump_status(){
	PROFILE_ME;

	DefenseBuilding::pump_status();

	check_auto_inc_resources();

	const auto utc_now = Poseidon::get_utc_time();

	bool dirty = false;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		if(check_building_mission(it->second, utc_now)){
			++dirty;
		}
	}
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		if(check_tech_mission(it->second, utc_now)){
			++dirty;
		}
	}
	if(dirty){
		recalculate_attributes(true);
		async_recheck_building_level_tasks(get_owner_uuid());
	}

	pump_population_production();
	pump_treatment();

	if(!is_buff_in_effect(BuffIds::ID_NOVICIATE_PROTECTION)){
		async_cancel_noviciate_protection(get_owner_uuid());
	}
}
void Castle::recalculate_attributes(bool recursive){
	PROFILE_ME;

	std::vector<boost::shared_ptr<MapCell>> child_cells;
	std::vector<boost::shared_ptr<MapObject>> child_objects;

	if(recursive){
		WorldMap::get_map_cells_by_parent_object(child_cells, get_map_object_uuid());
		WorldMap::get_map_objects_by_parent_object(child_objects, get_map_object_uuid());
	}

	// 使用旧的产率更新产出。
	for(auto it = child_cells.begin(); it != child_cells.end(); ++it){
		const auto &child_cell = *it;
		try {
			child_cell->pump_status();
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		if(child_object->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		try {
			child_object->pump_status();
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers.reserve(32);

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto &obj = it->second;
		const unsigned building_level = obj->get_building_level();
		if(building_level == 0){
			continue;
		}
		const auto building_id = BuildingId(obj->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);

		const auto upgrade_data = Data::CastleUpgradeAbstract::get(building_data->type, building_level);
		if(upgrade_data){
			auto &prosperity = modifiers[AttributeIds::ID_PROSPERITY_POINTS];
			prosperity += static_cast<std::int64_t>(upgrade_data->prosperity_points);

			if(building_data->type == BuildingTypeIds::ID_PRIMARY){
				auto &display_level = modifiers[AttributeIds::ID_BUILDING_LEVEL];
				if(display_level < building_level){
					display_level = building_level;
				}
			}
		}

		const auto upgrade_addon_data = Data::CastleUpgradeAddonAbstract::get(building_data->type, building_level);
		if(upgrade_addon_data){
			const auto &attributes = upgrade_addon_data->attributes;
			for(auto ait = attributes.begin(); ait != attributes.end(); ++ait){
				auto &value = modifiers[ait->first];
				value += std::round(ait->second * 1000.0);
			}
		}
	}
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		const auto &obj = it->second;
		const auto tech_id = TechId(obj->get_tech_id());
		const unsigned tech_level = obj->get_tech_level();
		if(tech_level == 0){
			continue;
		}
		const auto tech_data = Data::CastleTech::get(tech_id, tech_level);
		if(!tech_data){
			continue;
		}
		const auto &attributes = tech_data->attributes;
		for(auto ait = attributes.begin(); ait != attributes.end(); ++ait){
			auto &value = modifiers[ait->first];
			value += std::round(ait->second * 1000.0);
		}
	}

	// 提交新的属性。注意该行前后城堡属性发生变化。
	set_attributes(std::move(modifiers));

	DefenseBuilding::recalculate_attributes(recursive);

	// 更新新的部队属性。
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		if(child_object->get_map_object_type_id() == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		try {
			child_object->recalculate_attributes(false);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
}

void Castle::pump_population_production(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	if(!m_population_production_stamps){
		auto obj = boost::make_shared<MySql::Center_CastleBattalionProduction>(
			get_map_object_uuid().get(), 0, 0, 0, 0, utc_now, utc_now);
		obj->async_save(true);
		m_population_production_stamps = std::move(obj);
	}
	// 特殊：
	// production_time_begin 是上次人口消耗资源的时间。
	// production_time_end 是上次人口产出的时间。

	// 人口消耗。
	const auto last_consumption_time = m_population_production_stamps->get_production_time_begin();
	const auto consumption_minutes = saturated_sub(utc_now, last_consumption_time) / 60000;
	if(consumption_minutes > 0){
		const auto consumption_duration = consumption_minutes * 60000;
		LOG_EMPERY_CENTER_DEBUG("Checking population consumption: map_object_uuid = ", get_map_object_uuid(),
			", consumption_minutes = ", consumption_minutes);

		boost::container::flat_map<ResourceId, std::uint64_t> resources_to_consume;
		for(auto it = m_soldiers.begin(); it != m_soldiers.end(); ++it){
			const auto &obj = it->second;
			const auto soldier_count = obj->get_count();
			if(soldier_count <= 0){
				continue;
			}
			const auto map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
			const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
			if(!map_object_type_data){
				continue;
			}
			for(auto rit = map_object_type_data->maintenance_cost.begin(); rit != map_object_type_data->maintenance_cost.end(); ++rit){
				auto &amount_total = resources_to_consume[rit->first];
				amount_total = saturated_add(amount_total,
					static_cast<std::uint64_t>(std::ceil(static_cast<double>(consumption_minutes) * soldier_count * rit->second - 0.001)));
			}
		}
		std::vector<boost::shared_ptr<MapObject>> child_objects;
		WorldMap::get_map_objects_by_parent_object(child_objects, get_map_object_uuid());
		for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
			const auto &child_object = *it;
			const auto soldier_count = child_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT);
			if(soldier_count <= 0){
				continue;
			}
			const auto map_object_type_id = child_object->get_map_object_type_id();
			const auto map_object_type_data = Data::MapObjectTypeBattalion::get(map_object_type_id);
			if(!map_object_type_data){
				continue;
			}
			for(auto rit = map_object_type_data->maintenance_cost.begin(); rit != map_object_type_data->maintenance_cost.end(); ++rit){
				auto &amount_total = resources_to_consume[rit->first];
				amount_total = saturated_add(amount_total,
					static_cast<std::uint64_t>(std::ceil(static_cast<double>(consumption_minutes) * soldier_count * rit->second - 0.001)));
			}
		}
		std::vector<ResourceTransactionElement> transaction;
		transaction.reserve(resources_to_consume.size());
		for(auto it = resources_to_consume.begin(); it != resources_to_consume.end(); ++it){
			const auto resource_id = it->first;
			const auto amount_total = it->second;

			const auto resource_data = Data::CastleResource::require(resource_id);

			std::uint64_t amount_normal = amount_total, amount_token = 0;
			const auto resource_token_id = resource_data->resource_token_id;
			if(resource_token_id){
				const auto tokens_avail = get_resource(resource_token_id).amount;
				amount_token = std::min(amount_total, tokens_avail);
				amount_normal = saturated_sub(amount_total, amount_token);
			}
			if(amount_normal != 0){
				transaction.emplace_back(ResourceTransactionElement::OP_REMOVE_SATURATED, resource_id, amount_normal,
					ReasonIds::ID_POPULATION_CONSUMPTION, consumption_duration, 0, 0);
			}
			if(amount_token != 0){
				transaction.emplace_back(ResourceTransactionElement::OP_REMOVE_SATURATED, resource_token_id, amount_token,
					ReasonIds::ID_POPULATION_CONSUMPTION, consumption_duration, 0, 0);
			}
		}
		commit_resource_transaction(transaction,
			[&]{ m_population_production_stamps->set_production_time_begin(last_consumption_time + consumption_duration); });
	}

	// 人口产出。
	double production_rate = 0;
	double capacity        = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_CIVILIAN){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(current_level == 0){
			continue;
		}
		const auto upgrade_data = Data::CastleUpgradeCivilian::require(current_level);
		production_rate += upgrade_data->population_production_rate;
		capacity        += upgrade_data->population_capacity;
	}
	if(!is_garrisoned() && (production_rate > 0)){
		double tech_turbo;
		tech_turbo = get_attribute(AttributeIds::ID_PRODUCTION_TURBO_POPULATION) / 1000.0;
		production_rate *= (1 + tech_turbo);

		// const auto vip_data = Data::Vip::require(account->get_promotion_level());
		// production_rate *= (1 + vip_data->production_turbo);

		capacity = std::round(capacity);

		if(production_rate < 0){
			production_rate = 0;
		}
		if(capacity < 0){
			capacity = 0;
		}
		LOG_EMPERY_CENTER_TRACE("Checking population production: map_object_uuid = ", get_map_object_uuid(),
			", production_rate = ", production_rate, ", capacity = ", capacity);

		const auto old_last_production_time = m_population_production_stamps->get_production_time_end();
		const auto old_resource_amount      = get_resource(ResourceIds::ID_POPULATION).amount;

		const auto production_duration = saturated_sub(utc_now, old_last_production_time);
		const auto amount_produced = production_duration * production_rate / 60000.0 + m_population_production_remainder;
		const auto rounded_amount_produced = static_cast<std::uint64_t>(amount_produced);
		const auto new_resource_amount = std::min<std::uint64_t>(saturated_add(old_resource_amount, rounded_amount_produced), capacity);
		if(new_resource_amount > old_resource_amount){
			std::vector<ResourceTransactionElement> transaction;
			transaction.emplace_back(ResourceTransactionElement::OP_ADD, ResourceIds::ID_POPULATION, new_resource_amount - old_resource_amount,
				ReasonIds::ID_POPULATION_PRODUCTION, production_duration, 0, 0);
			commit_resource_transaction(transaction);
		}
		m_population_production_remainder = amount_produced - rounded_amount_produced;
	}
	m_population_production_stamps->set_production_time_end(utc_now);

	m_population_production_rate = production_rate;
	m_population_capacity        = capacity;
}

void Castle::check_init_buildings(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Checking init buildings: map_object_uuid = ", get_map_object_uuid());

	bool dirty = false;
	std::vector<boost::shared_ptr<const Data::CastleBuildingBase>> init_buildings;
	Data::CastleBuildingBase::get_init(init_buildings);
	for(auto dit = init_buildings.begin(); dit != init_buildings.end(); ++dit){
		const auto &building_data = *dit;
		const auto building_base_id = building_data->building_base_id;
		auto bit = m_buildings.find(building_base_id);
		if(bit == m_buildings.end()){
			auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
				get_map_object_uuid().get(), building_base_id.get(), 0, 0, MIS_NONE, 0, 0, 0);
			obj->async_save(true);
		}
		const auto &obj = bit->second;
		const auto building_id = obj->get_building_id();
		if(building_id){
			continue;
		}

		BuildingId init_building_id;
		const auto init_level = building_data->init_level;
		if(building_data->init_building_id_override){
			init_building_id = building_data->init_building_id_override;
		} else {
			const auto &buildings_allowed = building_data->buildings_allowed;
			boost::container::flat_map<BuildingId, unsigned> random_buildings;
			random_buildings.reserve(buildings_allowed.size());
			for(auto it = buildings_allowed.begin(); it != buildings_allowed.end(); ++it){
				const auto building_id = *it;
				random_buildings.emplace(building_id, 0);
			}
			for(auto tit = m_buildings.begin(); tit != m_buildings.end(); ++tit){
				const auto &obj = tit->second;
				const auto building_id = BuildingId(obj->get_building_id());
				const auto it = random_buildings.find(building_id);
				if(it == random_buildings.end()){
					continue;
				}
				it->second += 1;
			}
			{
				auto it = random_buildings.begin();
				while(it != random_buildings.end()){
					const auto random_building_data = Data::CastleBuilding::require(it->first);
					if(it->second >= random_building_data->build_limit){
						LOG_EMPERY_CENTER_DEBUG("> Build limit exceeded: building_id = ", it->first,
							", current_count = ", it->second, ", build_limit = ", random_building_data->build_limit);
						it = random_buildings.erase(it);
					} else {
						++it;
					}
				}
			}
			if(!random_buildings.empty()){
				const auto index = static_cast<std::ptrdiff_t>(Poseidon::rand32() % random_buildings.size());
				init_building_id = (random_buildings.begin() + index)->first;
			}
		}
		if(!init_building_id){
			continue;
		}

		LOG_EMPERY_CENTER_DEBUG("> Creating init building: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id,
			", init_building_id = ", init_building_id, ", init_level = ", init_level);
		obj->set_building_id(init_building_id.get());
		obj->set_building_level(init_level);
		++dirty;
	}
	if(dirty){
		async_recheck_building_level_tasks(get_owner_uuid());
	}
}

Castle::BuildingBaseInfo Castle::get_building_base(BuildingBaseId building_base_id) const {
	PROFILE_ME;

	BuildingBaseInfo info = { };
	info.building_base_id = building_base_id;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		return info;
	}
	fill_building_base_info(info, it->second);
	return info;
}
void Castle::get_all_building_bases(std::vector<Castle::BuildingBaseInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_buildings.size());
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		BuildingBaseInfo info;
		fill_building_base_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void Castle::get_buildings_by_id(std::vector<Castle::BuildingBaseInfo> &ret, BuildingId building_id) const {
	PROFILE_ME;

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto current_id = BuildingId(it->second->get_building_id());
		if(!current_id){
			continue;
		}
		if(current_id != building_id){
			continue;
		}
		BuildingBaseInfo info;
		fill_building_base_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void Castle::get_buildings_by_type_id(std::vector<Castle::BuildingBaseInfo> &ret, BuildingTypeId type) const {
	PROFILE_ME;

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto current_id = BuildingId(it->second->get_building_id());
		if(!current_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(current_id);
		if(building_data->type != type){
			continue;
		}
		BuildingBaseInfo info;
		fill_building_base_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::create_building_mission(BuildingBaseId building_base_id, Castle::Mission mission, std::uint64_t duration, BuildingId building_id){
	PROFILE_ME;

	auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
			get_map_object_uuid().get(), building_base_id.get(), 0, 0, MIS_NONE, 0, 0, 0);
		obj->async_save(true);
		it = m_buildings.emplace(building_base_id, obj).first;
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission != MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("Building mission conflict: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Building mission conflict"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	if(mission == MIS_CONSTRUCT){
		obj->set_building_id(building_id.get());
		obj->set_building_level(0);
	}
	obj->set_mission(mission);
	obj->set_mission_duration(duration);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(saturated_add(utc_now, duration));

	if(check_building_mission(obj, utc_now)){
		recalculate_attributes(true);
		pump_population_production();
		async_recheck_building_level_tasks(get_owner_uuid());
	}

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBuildingBase msg;
			fill_building_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::cancel_building_mission(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No building mission: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(0);
	obj->set_mission_time_end(0);
/*
	if(check_building_mission(obj, utc_now)){
		recalculate_attributes(true);
		pump_population_production();
		async_recheck_building_level_tasks(get_owner_uuid());
	}
*/
	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBuildingBase msg;
			fill_building_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::speed_up_building_mission(BuildingBaseId building_base_id, std::uint64_t delta_duration){
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_WARNING("Building base not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Building base not found"));
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No building mission: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("No building mission"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission_time_end(saturated_sub(obj->get_mission_time_end(), delta_duration));

	if(check_building_mission(obj, utc_now)){
		recalculate_attributes(true);
		pump_population_production();
		async_recheck_building_level_tasks(get_owner_uuid());
	}

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBuildingBase msg;
			fill_building_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::forced_replace_building(BuildingBaseId building_base_id, BuildingId building_id, unsigned building_level){
	PROFILE_ME;

	auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
			get_map_object_uuid().get(), building_base_id.get(), 0, 0, MIS_NONE, 0, 0, 0);
		obj->async_save(true);
		it = m_buildings.emplace(building_base_id, obj).first;
	}
	const auto &obj = it->second;

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_building_id(building_id.get());
	obj->set_building_level(building_level);
	obj->set_mission(0);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(utc_now);
/*
	if(check_building_mission(obj, utc_now)){
		recalculate_attributes(true);
		pump_population_production();
		async_recheck_building_level_tasks(get_owner_uuid());
	}
*/
	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBuildingBase msg;
			fill_building_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void Castle::pump_building_status(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	if(check_building_mission(it->second, utc_now)){
		recalculate_attributes(true);
		pump_population_production();
		async_recheck_building_level_tasks(get_owner_uuid());
	}
}
unsigned Castle::get_building_queue_size() const {
	PROFILE_ME;

	unsigned size = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto &obj = it->second;
		const auto mission = Mission(obj->get_mission());
		if(mission == MIS_NONE){
			continue;
		}
		++size;
	}
	return size;
}
void Castle::synchronize_building_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_CastleBuildingBase msg;
	fill_building_message(msg, it->second, utc_now);
	session->send(msg);
}

void Castle::accumulate_building_levels(boost::container::flat_map<BuildingId, boost::container::flat_map<unsigned, std::size_t>> &ret) const {
	PROFILE_ME;

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const unsigned level = it->second->get_building_level();
		auto &count = ret[building_id][level];
		++count;
	}
}

unsigned Castle::get_max_level(BuildingId building_id) const {
	PROFILE_ME;

	unsigned level = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto cur_building_id = BuildingId(it->second->get_building_id());
		if(cur_building_id != building_id){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(level >= current_level){
			continue;
		}
		level = current_level;
	}
	return level;
}
unsigned Castle::get_level() const {
	PROFILE_ME;

	unsigned level = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_PRIMARY){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(level >= current_level){
			continue;
		}
		level = current_level;
	}
	return level;
}
std::uint64_t Castle::get_warehouse_capacity(ResourceId resource_id) const {
	PROFILE_ME;

	std::uint64_t capacity = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_WAREHOUSE){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(current_level == 0){
			continue;
		}
		const auto upgrade_data = Data::CastleUpgradeWarehouse::require(current_level);
		const auto rit = upgrade_data->max_resource_amounts.find(resource_id);
		if(rit == upgrade_data->max_resource_amounts.end()){
			continue;
		}
		capacity = saturated_add(capacity, rit->second);
	}
	return capacity;
}
std::uint64_t Castle::get_warehouse_protection(ResourceId resource_id) const {
	PROFILE_ME;

	std::uint64_t protection = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_WAREHOUSE){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(current_level == 0){
			continue;
		}
		const auto upgrade_data = Data::CastleUpgradeWarehouse::require(current_level);
		const auto rit = upgrade_data->protected_resource_amounts.find(resource_id);
		if(rit == upgrade_data->protected_resource_amounts.end()){
			continue;
		}
		protection = saturated_add(protection, rit->second);
	}
	return protection;
}
bool Castle::is_tech_upgrade_in_progress() const {
	PROFILE_ME;

	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		const auto &obj = it->second;
		const auto mission = Mission(obj->get_mission());
		if(mission != MIS_NONE){
			return true;
		}
	}
	return false;
}
bool Castle::is_soldier_production_in_progress(BuildingBaseId building_base_id) const {
	PROFILE_ME;

	const auto it = m_soldier_production.find(building_base_id);
	if(it != m_soldier_production.end()){
		const auto &obj = it->second;
		const auto map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
		if(map_object_type_id != MapObjectTypeId()){
			return true;
		}
	}
	return false;
}
std::uint64_t Castle::get_max_battalion_count() const {
	PROFILE_ME;

	std::uint64_t count = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_PARADE_GROUND){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(current_level == 0){
			continue;
		}
		const auto upgrade_data = Data::CastleUpgradeParadeGround::require(current_level);
		count = saturated_add(count, upgrade_data->max_battalion_count);
	}
	return count;
}
std::uint64_t Castle::get_max_soldier_count_bonus() const {
	PROFILE_ME;

	std::uint64_t count = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_PARADE_GROUND){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(current_level == 0){
			continue;
		}
		const auto upgrade_data = Data::CastleUpgradeParadeGround::require(current_level);
		count = saturated_add(count, upgrade_data->max_soldier_count_bonus);
	}
	return count;
}
std::uint64_t Castle::get_medical_tent_capacity() const {
	PROFILE_ME;

	std::uint64_t count = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(!building_id){
			continue;
		}
		const auto building_data = Data::CastleBuilding::require(building_id);
		if(building_data->type != BuildingTypeIds::ID_MEDICAL_TENT){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		if(current_level == 0){
			continue;
		}
		const auto upgrade_data = Data::CastleUpgradeMedicalTent::require(current_level);
		count = saturated_add(count, upgrade_data->capacity);
	}
	return count;
}
bool Castle::is_treatment_in_progress() const {
	PROFILE_ME;

	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		const auto count = obj->get_count();
		if(count != 0){
			return true;
		}
	}
	return false;
}

Castle::TechInfo Castle::get_tech(TechId tech_id) const {
	PROFILE_ME;

	TechInfo info = { };
	info.tech_id = tech_id;

	const auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		return info;
	}
	fill_tech_info(info, it->second);
	return info;
}
void Castle::get_all_techs(std::vector<Castle::TechInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_techs.size());
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		TechInfo info;
		fill_tech_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::create_tech_mission(TechId tech_id, Castle::Mission mission, std::uint64_t duration){
	PROFILE_ME;

	auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		auto obj = boost::make_shared<MySql::Center_CastleTech>(
			get_map_object_uuid().get(), tech_id.get(), 0, MIS_NONE, 0, 0, 0);
		obj->async_save(true);
		it = m_techs.emplace(tech_id, obj).first;
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission != MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("Tech mission conflict: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		DEBUG_THROW(Exception, sslit("Tech mission conflict"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(mission);
	obj->set_mission_duration(duration);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(saturated_add(utc_now, duration));

	if(check_tech_mission(obj, utc_now)){
		recalculate_attributes(true);
	}

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTech msg;
			fill_tech_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::cancel_tech_mission(TechId tech_id){
	PROFILE_ME;

	const auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_DEBUG("Tech not found: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		return;
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No tech mission: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(0);
	obj->set_mission_time_end(0);
/*
	if(check_tech_mission(obj, utc_now)){
		recalculate_attributes(true);
	}
*/
	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTech msg;
			fill_tech_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::speed_up_tech_mission(TechId tech_id, std::uint64_t delta_duration){
	PROFILE_ME;

	const auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_WARNING("Tech not found: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		DEBUG_THROW(Exception, sslit("Tech not found"));
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No tech mission: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		DEBUG_THROW(Exception, sslit("No tech mission"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission_time_end(saturated_sub(obj->get_mission_time_end(), delta_duration));

	if(check_tech_mission(obj, utc_now)){
		recalculate_attributes(true);
	}

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTech msg;
			fill_tech_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::forced_replace_tech(TechId tech_id, unsigned tech_level){
	PROFILE_ME;

	auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		auto obj = boost::make_shared<MySql::Center_CastleTech>(
			get_map_object_uuid().get(), tech_id.get(), 0, MIS_NONE, 0, 0, 0);
		obj->async_save(true);
		it = m_techs.emplace(tech_id, obj).first;
	}
	const auto &obj = it->second;

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_tech_level(tech_level);
	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(utc_now);
/*
	if(check_tech_mission(obj, utc_now)){
		recalculate_attributes(true);
	}
*/
	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTech msg;
			fill_tech_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void Castle::pump_tech_status(TechId tech_id){
	PROFILE_ME;

	const auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_DEBUG("Tech not found: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	if(check_tech_mission(it->second, utc_now)){
		recalculate_attributes(true);
	}
}
unsigned Castle::get_tech_queue_size() const {
	PROFILE_ME;

	unsigned size = 0;
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		const auto &obj = it->second;
		const auto mission = Mission(obj->get_mission());
		if(mission == MIS_NONE){
			continue;
		}
		++size;
	}
	return size;
}
void Castle::synchronize_tech_with_player(TechId tech_id, const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_DEBUG("Tech not found: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_CastleTech msg;
	fill_tech_message(msg, it->second, utc_now);
	session->send(msg);
}

void Castle::check_init_resources(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Checking init resources: map_object_uuid = ", get_map_object_uuid());

	std::vector<ResourceTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::CastleResource>> resource_data_all;
	Data::CastleResource::get_all(resource_data_all);
	for(auto dit = resource_data_all.begin(); dit != resource_data_all.end(); ++dit){
		const auto &resource_data = *dit;
		const auto resource_id = resource_data->resource_id;
		const auto rit = m_resources.find(resource_id);
		if(rit != m_resources.end()){
			continue;
		}
		LOG_EMPERY_CENTER_TRACE("> Adding resource: resource_id = ", resource_id, ", init_amount = ", resource_data->init_amount);
		transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, resource_data->init_amount,
			ReasonIds::ID_INIT_RESOURCES, resource_data->init_amount, 0, 0);
	}
	commit_resource_transaction(transaction);
}
void Castle::check_auto_inc_resources(){
	PROFILE_ME;
	LOG_EMPERY_CENTER_TRACE("Checking auto increment resources: map_object_uuid = ", get_map_object_uuid());

	const auto utc_now = Poseidon::get_utc_time();

	std::vector<ResourceTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::CastleResource>> resources_to_check;
	Data::CastleResource::get_auto_inc(resources_to_check);
	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleResource>, std::uint64_t> new_timestamps;
	for(auto dit = resources_to_check.begin(); dit != resources_to_check.end(); ++dit){
		const auto &resource_data = *dit;
		const auto resource_id = resource_data->resource_id;
		auto it = m_resources.find(resource_id);
		if(it == m_resources.end()){
			auto obj = boost::make_shared<MySql::Center_CastleResource>(get_map_object_uuid().get(), resource_id.get(), 0, 0);
			obj->async_save(true);
			it = m_resources.emplace(resource_id, std::move(obj)).first;
		}
		const auto &obj = it->second;

		std::uint64_t auto_inc_period, auto_inc_offset;
		switch(resource_data->auto_inc_type){
		case Data::CastleResource::AIT_HOURLY:
			auto_inc_period = 3600 * 1000;
			auto_inc_offset = checked_mul<std::uint64_t>(resource_data->auto_inc_offset, 60000);
			break;
		case Data::CastleResource::AIT_DAILY:
			auto_inc_period = 24 * 3600 * 1000;
			auto_inc_offset = checked_mul<std::uint64_t>(resource_data->auto_inc_offset, 60000);
			break;
		case Data::CastleResource::AIT_WEEKLY:
			auto_inc_period = 7 * 24 * 3600 * 1000;
			auto_inc_offset = checked_mul<std::uint64_t>(resource_data->auto_inc_offset, 60000) + 3 * 24 * 3600 * 1000; // 注意 1970-01-01 是星期四。
			break;
		case Data::CastleResource::AIT_PERIODIC:
			auto_inc_period = checked_mul<std::uint64_t>(resource_data->auto_inc_offset, 60000);
			auto_inc_offset = utc_now + 1; // 当前时间永远是区间中的最后一秒。
			break;
		default:
			auto_inc_period = 0;
			auto_inc_offset = 0;
			break;
		}
		if(auto_inc_period == 0){
			LOG_EMPERY_CENTER_WARNING("Castle resource auto increment period is zero? resource_id = ", resource_id);
			continue;
		}
		auto_inc_offset %= auto_inc_period;

		const auto old_amount = obj->get_amount();
		const auto old_updated_time = obj->get_updated_time();

		const auto prev_interval = checked_sub(checked_add(old_updated_time, auto_inc_period), auto_inc_offset) / auto_inc_period;
		const auto cur_interval = checked_sub(utc_now, auto_inc_offset) / auto_inc_period;
		LOG_EMPERY_CENTER_TRACE("> Checking resource: resource_id = ", resource_id,
			", prev_interval = ", prev_interval, ", cur_interval = ", cur_interval);
		if(cur_interval <= prev_interval){
			continue;
		}
		const auto interval_count = cur_interval - prev_interval;

		if(resource_data->auto_inc_step >= 0){
			if(old_amount < resource_data->auto_inc_bound){
				const auto amount_to_add = saturated_mul(static_cast<std::uint64_t>(resource_data->auto_inc_step), interval_count);
				const auto new_amount = std::min(saturated_add(old_amount, amount_to_add), resource_data->auto_inc_bound);
				LOG_EMPERY_CENTER_TRACE("> Adding resource: resource_id = ", resource_id,
					", old_amount = ", old_amount, ", new_amount = ", new_amount);
				transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, new_amount - old_amount,
					ReasonIds::ID_AUTO_INCREMENT, resource_data->auto_inc_type, resource_data->auto_inc_offset, 0);
			}
		} else {
			if(old_amount > resource_data->auto_inc_bound){
				const auto amount_to_remove = saturated_mul(static_cast<std::uint64_t>(-(resource_data->auto_inc_step)), interval_count);
				const auto new_amount = std::max(saturated_sub(old_amount, amount_to_remove), resource_data->auto_inc_bound);
				LOG_EMPERY_CENTER_TRACE("> Removing resource: resource_id = ", resource_id,
					", old_amount = ", old_amount, ", new_amount = ", new_amount);
				transaction.emplace_back(ResourceTransactionElement::OP_REMOVE, resource_id, old_amount - new_amount,
					ReasonIds::ID_AUTO_INCREMENT, resource_data->auto_inc_type, resource_data->auto_inc_offset, 0);
			}
		}
		const auto new_updated_time = saturated_add(old_updated_time, saturated_mul(auto_inc_period, interval_count));
		new_timestamps.emplace(obj, new_updated_time);
	}
	commit_resource_transaction(transaction,
		[&]{
			for(auto it = new_timestamps.begin(); it != new_timestamps.end(); ++it){
				it->first->set_updated_time(it->second);
			}
		});
}

Castle::ResourceInfo Castle::get_resource(ResourceId resource_id) const {
	PROFILE_ME;

	ResourceInfo info = { };
	info.resource_id = resource_id;

	const auto it = m_resources.find(resource_id);
	if(it == m_resources.end()){
		return info;
	}
	fill_resource_info(info, it->second);
	return info;
}
void Castle::get_all_resources(std::vector<Castle::ResourceInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_resources.size());
	for(auto it = m_resources.begin(); it != m_resources.end(); ++it){
		ResourceInfo info;
		fill_resource_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

ResourceId Castle::commit_resource_transaction_nothrow(const std::vector<ResourceTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	std::vector<boost::shared_ptr<Poseidon::EventBaseWithoutId>> events;
	events.reserve(transaction.size());
	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleResource>, std::uint64_t /* new_amount */> temp_result_map;
	temp_result_map.reserve(transaction.size());

	const FlagGuard transaction_guard(m_locked_by_resource_transaction);

	const auto map_object_uuid = get_map_object_uuid();
	const auto owner_uuid = get_owner_uuid();

    for(auto tit = transaction.begin(); tit != transaction.end(); ++tit){
		const auto operation    = tit->m_operation;
		const auto resource_id  = tit->m_some_id;
		const auto delta_amount = tit->m_delta_count;

		if(delta_amount == 0){
			continue;
		}

		const auto reason = tit->m_reason;
		const auto param1 = tit->m_param1;
		const auto param2 = tit->m_param2;
		const auto param3 = tit->m_param3;

		switch(operation){
		case ResourceTransactionElement::OP_NONE:
			break;

		case ResourceTransactionElement::OP_ADD:
			{
				boost::shared_ptr<MySql::Center_CastleResource> obj;
				{
					const auto it = m_resources.find(resource_id);
					if(it == m_resources.end()){
						obj = boost::make_shared<MySql::Center_CastleResource>(get_map_object_uuid().get(), resource_id.get(), 0, 0);
						obj->async_save(true);
						m_resources.emplace(resource_id, obj);
					} else {
						obj = it->second;
					}
				}
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_amount());
				}
				const auto old_amount = temp_it->second;
				temp_it->second = checked_add(old_amount, delta_amount);
				const auto new_amount = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("@ Resource transaction: add: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", resource_id = ", resource_id,
					", old_amount = ", old_amount, ", delta_amount = ", delta_amount, ", new_amount = ", new_amount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::ResourceChanged>(
					map_object_uuid, owner_uuid, resource_id, old_amount, new_amount, reason, param1, param2, param3));
			}
			break;

		case ResourceTransactionElement::OP_REMOVE:
		case ResourceTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_resources.find(resource_id);
				if(it == m_resources.end()){
					if(operation != ResourceTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Resource not found: resource_id = ", resource_id);
						return resource_id;
					}
					break;
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_amount());
				}
				const auto old_amount = temp_it->second;
				if(temp_it->second >= delta_amount){
					temp_it->second -= delta_amount;
				} else {
					if(operation != ResourceTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough resources: resource_id = ", resource_id,
							", temp_amount = ", temp_it->second, ", delta_amount = ", delta_amount);
						return resource_id;
					}
					temp_it->second = 0;
				}
				const auto new_amount = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("@ Resource transaction: remove: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", resource_id = ", resource_id,
					", old_amount = ", old_amount, ", delta_amount = ", delta_amount, ", new_amount = ", new_amount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::ResourceChanged>(
					map_object_uuid, owner_uuid, resource_id, old_amount, new_amount, reason, param1, param2, param3));
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown resource transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown resource transaction operation"));
		}
	}

	const auto withdrawn = boost::make_shared<bool>(true);
	for(auto it = events.begin(); it != events.end(); ++it){
		Poseidon::async_raise_event(*it, withdrawn);
	}
	if(callback){
		callback();
	}
	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_amount(it->second);
	}
	*withdrawn = false;

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
				Msg::SC_CastleResource msg;
				fill_resource_message(msg, it->first);
				session->send(msg);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return { };
}
void Castle::commit_resource_transaction(const std::vector<ResourceTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuff_id = commit_resource_transaction_nothrow(transaction, callback);
	if(insuff_id != ResourceId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient resources in castle: map_object_uuid = ", get_map_object_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient resources in castle"));
	}
}

Castle::SoldierInfo Castle::get_soldier(MapObjectTypeId map_object_type_id) const {
	PROFILE_ME;

	SoldierInfo info = { };
	info.map_object_type_id = map_object_type_id;

	const auto it = m_soldiers.find(map_object_type_id);
	if(it == m_soldiers.end()){
		return info;
	}
	fill_soldier_info(info, it->second);
	return info;
}
void Castle::get_all_soldiers(std::vector<Castle::SoldierInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_soldiers.size());
	for(auto it = m_soldiers.begin(); it != m_soldiers.end(); ++it){
		SoldierInfo info;
		fill_soldier_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void Castle::enable_soldier(MapObjectTypeId map_object_type_id){
	PROFILE_ME;

	auto it = m_soldiers.find(map_object_type_id);
	if(it == m_soldiers.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBattalion>(
			get_map_object_uuid().get(), map_object_type_id.get(), 0, false);
		obj->async_save(true);
		it = m_soldiers.emplace(map_object_type_id, std::move(obj)).first;
	}
	if(it->second->get_enabled()){
		LOG_EMPERY_CENTER_DEBUG("Soldier is already enabled: map_object_uuid = ", get_map_object_uuid().get(),
			", map_object_type_id = ", map_object_type_id);
		return;
	}

	it->second->set_enabled(true);

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleSoldier msg;
			fill_soldier_message(msg, it->second);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

MapObjectTypeId Castle::commit_soldier_transaction_nothrow(const std::vector<SoldierTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	std::vector<boost::shared_ptr<Poseidon::EventBaseWithoutId>> events;
	events.reserve(transaction.size());
	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleBattalion>, std::uint64_t /* new_count */> temp_result_map;
	temp_result_map.reserve(transaction.size());

	const FlagGuard transaction_guard(m_locked_by_soldier_transaction);

	const auto map_object_uuid = get_map_object_uuid();
	const auto owner_uuid = get_owner_uuid();

    for(auto tit = transaction.begin(); tit != transaction.end(); ++tit){
		const auto operation          = tit->m_operation;
		const auto map_object_type_id = tit->m_some_id;
		const auto delta_count        = tit->m_delta_count;

		if(delta_count == 0){
			continue;
		}

		const auto reason = tit->m_reason;
		const auto param1 = tit->m_param1;
		const auto param2 = tit->m_param2;
		const auto param3 = tit->m_param3;

		switch(operation){
		case SoldierTransactionElement::OP_NONE:
			break;

		case SoldierTransactionElement::OP_ADD:
			{
				boost::shared_ptr<MySql::Center_CastleBattalion> obj;
				{
					const auto it = m_soldiers.find(map_object_type_id);
					if(it == m_soldiers.end()){
						obj = boost::make_shared<MySql::Center_CastleBattalion>(
							get_map_object_uuid().get(), map_object_type_id.get(), 0, false);
						obj->enable_auto_saving(); // obj->async_save(true);
						m_soldiers.emplace(map_object_type_id, obj);
					} else {
						obj = it->second;
					}
				}
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				temp_it->second = checked_add(old_count, delta_count);
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("@ Soldier transaction: add: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", map_object_type_id = ", map_object_type_id,
					", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::SoldierChanged>(
					map_object_uuid, owner_uuid, map_object_type_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		case SoldierTransactionElement::OP_REMOVE:
		case SoldierTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_soldiers.find(map_object_type_id);
				if(it == m_soldiers.end()){
					if(operation != SoldierTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Soldier not found: map_object_type_id = ", map_object_type_id);
						return map_object_type_id;
					}
					break;
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				if(temp_it->second >= delta_count){
					temp_it->second -= delta_count;
				} else {
					if(operation != SoldierTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough soldiers: map_object_type_id = ", map_object_type_id,
							", temp_count = ", temp_it->second, ", delta_count = ", delta_count);
						return map_object_type_id;
					}
					temp_it->second = 0;
				}
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("@ Soldier transaction: remove: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", map_object_type_id = ", map_object_type_id,
					", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::SoldierChanged>(
					map_object_uuid, owner_uuid, map_object_type_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown soldier transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown soldier transaction operation"));
		}
	}

	const auto withdrawn = boost::make_shared<bool>(true);
	for(auto it = events.begin(); it != events.end(); ++it){
		Poseidon::async_raise_event(*it, withdrawn);
	}
	if(callback){
		callback();
	}
	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_count(it->second);
	}
	*withdrawn = false;

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
				Msg::SC_CastleSoldier msg;
				fill_soldier_message(msg, it->first);
				session->send(msg);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return { };
}
void Castle::commit_soldier_transaction(const std::vector<SoldierTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuff_id = commit_soldier_transaction_nothrow(transaction, callback);
	if(insuff_id != MapObjectTypeId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient soldiers in castle: map_object_uuid = ", get_map_object_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient soldiers in castle"));
	}
}

Castle::SoldierProductionInfo Castle::get_soldier_production(BuildingBaseId building_base_id) const {
	PROFILE_ME;

	SoldierProductionInfo info = { };
	info.building_base_id = building_base_id;

	const auto it = m_soldier_production.find(building_base_id);
	if(it == m_soldier_production.end()){
		return info;
	}
	fill_soldier_production_info(info, it->second);
	return info;
}
void Castle::get_all_soldier_production(std::vector<Castle::SoldierProductionInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_soldier_production.size());
	for(auto it = m_soldier_production.begin(); it != m_soldier_production.end(); ++it){
		SoldierProductionInfo info;
		fill_soldier_production_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::begin_soldier_production(BuildingBaseId building_base_id,
	MapObjectTypeId map_object_type_id, std::uint64_t count, std::uint64_t duration)
{
	PROFILE_ME;

	auto it = m_soldier_production.find(building_base_id);
	if(it == m_soldier_production.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBattalionProduction>(
			get_map_object_uuid().get(), building_base_id.get(), 0, 0, 0, 0, 0);
		obj->async_save(true);
		it = m_soldier_production.emplace(building_base_id, obj).first;
	}
	const auto &obj = it->second;
	const auto old_map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(old_map_object_type_id != MapObjectTypeId()){
		LOG_EMPERY_CENTER_DEBUG("Soldier production conflict: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Soldier production conflict"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_map_object_type_id(map_object_type_id.get());
	obj->set_count(count);
	obj->set_production_duration(duration);
	obj->set_production_time_begin(utc_now);
	obj->set_production_time_end(saturated_add(utc_now, duration));

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleSoldierProduction msg;
			fill_soldier_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::cancel_soldier_production(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_soldier_production.find(building_base_id);
	if(it == m_soldier_production.end()){
		LOG_EMPERY_CENTER_DEBUG("Soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}
	const auto &obj = it->second;
	const auto old_map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(old_map_object_type_id == MapObjectTypeId()){
		LOG_EMPERY_CENTER_DEBUG("No soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_map_object_type_id(0);
	obj->set_count(0);
	obj->set_production_duration(0);
	obj->set_production_time_begin(utc_now);
	obj->set_production_time_end(utc_now);

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleSoldierProduction msg;
			fill_soldier_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::speed_up_soldier_production(BuildingBaseId building_base_id, std::uint64_t delta_duration){
	PROFILE_ME;

	const auto it = m_soldier_production.find(building_base_id);
	if(it == m_soldier_production.end()){
		LOG_EMPERY_CENTER_WARNING("Soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Soldier production not found"));
	}
	const auto &obj = it->second;
	const auto old_map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(old_map_object_type_id == MapObjectTypeId()){
		LOG_EMPERY_CENTER_WARNING("No soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Soldier production not found"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_production_time_end(saturated_sub(obj->get_production_time_end(), delta_duration));

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleSoldierProduction msg;
			fill_soldier_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

std::uint64_t Castle::harvest_soldier(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_soldier_production.find(building_base_id);
	if(it == m_soldier_production.end()){
		LOG_EMPERY_CENTER_WARNING("Soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Soldier production not found"));
	}
	const auto &obj = it->second;
	const auto map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(map_object_type_id == MapObjectTypeId()){
		LOG_EMPERY_CENTER_WARNING("No soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Soldier production not found"));
	}
	const auto count = obj->get_count();

	const auto utc_now = Poseidon::get_utc_time();

	if(utc_now < obj->get_production_time_end()){
		LOG_EMPERY_CENTER_WARNING("Soldier production incomplete: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Soldier production incomplete"));
	}

	std::vector<SoldierTransactionElement> transaction;
	transaction.emplace_back(SoldierTransactionElement::OP_ADD, map_object_type_id, count,
		ReasonIds::ID_HARVEST_BATTALION, map_object_type_id.get(), count, 0);
	commit_soldier_transaction(transaction,
		[&]{
			obj->set_map_object_type_id(0);
			obj->set_count(0);
			obj->set_production_duration(0);
			obj->set_production_time_begin(utc_now);
			obj->set_production_time_end(utc_now);
		});

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleSoldierProduction msg;
			fill_soldier_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return count;
}
void Castle::synchronize_soldier_production_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto it = m_soldier_production.find(building_base_id);
	if(it == m_soldier_production.end()){
		LOG_EMPERY_CENTER_DEBUG("Soldier production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_CastleSoldierProduction msg;
	fill_soldier_production_message(msg, it->second, utc_now);
	session->send(msg);
}

bool Castle::has_wounded_soldiers() const {
	PROFILE_ME;

	for(auto it = m_wounded_soldiers.begin(); it != m_wounded_soldiers.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_count() != 0){
			return true;
		}
	}
	return false;
}
Castle::WoundedSoldierInfo Castle::get_wounded_soldier(MapObjectTypeId map_object_type_id) const {
	PROFILE_ME;

	WoundedSoldierInfo info = { };
	info.map_object_type_id = map_object_type_id;

	const auto it = m_wounded_soldiers.find(map_object_type_id);
	if(it == m_wounded_soldiers.end()){
		return info;
	}
	fill_wounded_soldier_info(info, it->second);
	return info;
}
void Castle::get_all_wounded_soldiers(std::vector<Castle::WoundedSoldierInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_wounded_soldiers.size());
	for(auto it = m_wounded_soldiers.begin(); it != m_wounded_soldiers.end(); ++it){
		WoundedSoldierInfo info;
		fill_wounded_soldier_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

MapObjectTypeId Castle::commit_wounded_soldier_transaction_nothrow(const std::vector<WoundedSoldierTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	std::vector<boost::shared_ptr<Poseidon::EventBaseWithoutId>> events;
	events.reserve(transaction.size());
	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleWoundedSoldier>, std::uint64_t /* new_count */> temp_result_map;
	temp_result_map.reserve(transaction.size());

	const FlagGuard transaction_guard(m_locked_by_wounded_soldier_transaction);

	const auto map_object_uuid = get_map_object_uuid();
	const auto owner_uuid = get_owner_uuid();

    for(auto tit = transaction.begin(); tit != transaction.end(); ++tit){
		const auto operation          = tit->m_operation;
		const auto map_object_type_id = tit->m_some_id;
		const auto delta_count        = tit->m_delta_count;

		if(delta_count == 0){
			continue;
		}

		const auto reason = tit->m_reason;
		const auto param1 = tit->m_param1;
		const auto param2 = tit->m_param2;
		const auto param3 = tit->m_param3;

		switch(operation){
		case WoundedSoldierTransactionElement::OP_NONE:
			break;

		case WoundedSoldierTransactionElement::OP_ADD:
			{
				boost::shared_ptr<MySql::Center_CastleWoundedSoldier> obj;
				{
					const auto it = m_wounded_soldiers.find(map_object_type_id);
					if(it == m_wounded_soldiers.end()){
						obj = boost::make_shared<MySql::Center_CastleWoundedSoldier>(
							get_map_object_uuid().get(), map_object_type_id.get(), 0);
						obj->enable_auto_saving(); // obj->async_save(true);
						m_wounded_soldiers.emplace(map_object_type_id, obj);
					} else {
						obj = it->second;
					}
				}
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				temp_it->second = checked_add(old_count, delta_count);
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("* Wounded soldier transaction: add: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", map_object_type_id = ", map_object_type_id,
					", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::WoundedSoldierChanged>(
					map_object_uuid, owner_uuid, map_object_type_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		case WoundedSoldierTransactionElement::OP_REMOVE:
		case WoundedSoldierTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_wounded_soldiers.find(map_object_type_id);
				if(it == m_wounded_soldiers.end()){
					if(operation != WoundedSoldierTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Wounded soldier not found: map_object_type_id = ", map_object_type_id);
						return map_object_type_id;
					}
					break;
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				if(temp_it->second >= delta_count){
					temp_it->second -= delta_count;
				} else {
					if(operation != WoundedSoldierTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough wounded soldiers: map_object_type_id = ", map_object_type_id,
							", temp_count = ", temp_it->second, ", delta_count = ", delta_count);
						return map_object_type_id;
					}
					temp_it->second = 0;
				}
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("* Wounded soldier transaction: remove: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", map_object_type_id = ", map_object_type_id,
					", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::WoundedSoldierChanged>(
					map_object_uuid, owner_uuid, map_object_type_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown wounded soldier transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown wounded soldier transaction operation"));
		}
	}

	const auto withdrawn = boost::make_shared<bool>(true);
	for(auto it = events.begin(); it != events.end(); ++it){
		Poseidon::async_raise_event(*it, withdrawn);
	}
	if(callback){
		callback();
	}
	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_count(it->second);
	}
	*withdrawn = false;

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
				Msg::SC_CastleWoundedSoldier msg;
				fill_wounded_soldier_message(msg, it->first);
				session->send(msg);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return { };
}
void Castle::commit_wounded_soldier_transaction(const std::vector<WoundedSoldierTransactionElement> &transaction,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuff_id = commit_wounded_soldier_transaction_nothrow(transaction, callback);
	if(insuff_id != MapObjectTypeId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient wounded_soldiers in castle: map_object_uuid = ", get_map_object_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient wounded_soldiers in castle"));
	}
}

void Castle::pump_treatment(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_time_end() == 0){
			continue;
		}
		check_treatment_mission(obj, utc_now);
	}

	harvest_treatment();

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTreatment msg;
			fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void Castle::get_treatment(std::vector<Castle::TreatmentInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_treatment.size());
	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_time_end() == 0){
			continue;
		}
		TreatmentInfo info;
		fill_treatment_info(info, obj);
		ret.emplace_back(std::move(info));
	}
}
void Castle::begin_treatment(const boost::container::flat_map<MapObjectTypeId, std::uint64_t> &soldiers, std::uint64_t duration){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_time_end() == 0){
			continue;
		}
		LOG_EMPERY_CENTER_WARNING("Soldier treatment in progress: map_object_uuid = ", get_map_object_uuid(),
			", map_object_type_id = ", it->first, ", time_end = ", obj->get_time_end());
		DEBUG_THROW(Exception, sslit("Soldier treatment in progress"));
	}

	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleTreatment>, std::uint64_t> temp_result;
	temp_result.reserve(soldiers.size());

	m_treatment.reserve(m_treatment.size() + soldiers.size());
	for(auto it = soldiers.begin(); it != soldiers.end(); ++it){
		const auto map_object_type_id = it->first;
		auto iit = m_treatment.find(map_object_type_id);
		if(iit == m_treatment.end()){
			auto obj = boost::make_shared<MySql::Center_CastleTreatment>(
				get_map_object_uuid().get(), map_object_type_id.get(), 0, 0, 0, 0);
			obj->async_save(true);
			iit = m_treatment.emplace(map_object_type_id, std::move(obj)).first;
		}
		const auto &obj = iit->second;
		temp_result.emplace(obj, it->second);
	}

	for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
		const auto &obj = it->first;
		obj->set_count      (it->second);
		obj->set_duration   (duration);
		obj->set_time_begin (utc_now);
		obj->set_time_end   (saturated_add(utc_now, duration));
	}

	for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
		const auto &obj = it->first;
		check_treatment_mission(obj, utc_now);
	}

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTreatment msg;
			fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::cancel_treatment(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_time_end() == 0){
			continue;
		}
		obj->set_count(0);
		obj->set_duration(0);
		obj->set_time_begin(0);
		obj->set_time_end(0);

		// check_treatment_mission(obj, utc_now);
	}

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTreatment msg;
			fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::speed_up_treatment(std::uint64_t delta_duration){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		if(obj->get_time_end() == 0){
			continue;
		}
		obj->set_time_end(saturated_sub(obj->get_time_end(), delta_duration));

		check_treatment_mission(obj, utc_now);
	}

	harvest_treatment();

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTreatment msg;
			fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void Castle::harvest_treatment(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleTreatment>, std::uint64_t> temp_result;
	for(auto it = m_treatment.begin(); it != m_treatment.end(); ++it){
		const auto &obj = it->second;
		const auto count = obj->get_count();
		if(count == 0){
			continue;
		}
		const auto time_end = obj->get_time_end();
		if(utc_now < time_end){
			continue;
		}
		const auto map_object_type_id = it->first;
		const auto wounded_info = get_wounded_soldier(map_object_type_id);
		const auto count_harvested = std::min<std::uint64_t>(count, wounded_info.count);

		temp_result.emplace(obj, count_harvested);
	}

	std::vector<WoundedSoldierTransactionElement> wounded_transaction;
	std::vector<SoldierTransactionElement> soldier_transaction;
	for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
		const auto &obj = it->first;
		const auto map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
		const auto count_harvested = it->second;
		wounded_transaction.emplace_back(WoundedSoldierTransactionElement::OP_REMOVE, map_object_type_id, count_harvested,
			ReasonIds::ID_SOLDIER_HEALED, 0, 0, 0);
		soldier_transaction.emplace_back(SoldierTransactionElement::OP_ADD, map_object_type_id, count_harvested,
			ReasonIds::ID_SOLDIER_HEALED, 0, 0, 0);
	}
	commit_wounded_soldier_transaction(wounded_transaction,
		[&]{
			commit_soldier_transaction(soldier_transaction,
				[&]{
					for(auto it = temp_result.begin(); it != temp_result.end(); ++it){
						const auto &obj = it->first;
						obj->set_count(0);
					}
				});
		});

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleTreatment msg;
			fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::synchronize_treatment_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_CastleTreatment msg;
	fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
	session->send(msg);
}

void Castle::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		Msg::SC_CastleBuildingBase msg;
		fill_building_message(msg, it->second, utc_now);
		session->send(msg);
	}
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		Msg::SC_CastleTech msg;
		fill_tech_message(msg, it->second, utc_now);
		session->send(msg);
	}
	for(auto it = m_resources.begin(); it != m_resources.end(); ++it){
		Msg::SC_CastleResource msg;
		fill_resource_message(msg, it->second);
		session->send(msg);
	}
	for(auto it = m_soldiers.begin(); it != m_soldiers.end(); ++it){
		Msg::SC_CastleSoldier msg;
		fill_soldier_message(msg, it->second);
		session->send(msg);
	}
	for(auto it = m_soldier_production.begin(); it != m_soldier_production.end(); ++it){
		Msg::SC_CastleSoldierProduction msg;
		fill_soldier_production_message(msg, it->second, utc_now);
		session->send(msg);
	}
	for(auto it = m_wounded_soldiers.begin(); it != m_wounded_soldiers.end(); ++it){
		Msg::SC_CastleWoundedSoldier msg;
		fill_wounded_soldier_message(msg, it->second);
		session->send(msg);
	}
	{
		Msg::SC_CastleTreatment msg;
		fill_treatment_message(msg, get_map_object_uuid(), m_treatment, utc_now);
		session->send(msg);
	}
}

}
