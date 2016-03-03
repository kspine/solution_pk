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
	void fill_battalion_info(Castle::BattalionInfo &info, const boost::shared_ptr<MySql::Center_CastleBattalion> &obj){
		PROFILE_ME;

		info.map_object_type_id    = MapObjectTypeId(obj->get_map_object_type_id());
		info.count                 = obj->get_count();
		info.enabled               = obj->get_enabled();
	}
	void fill_battalion_production_info(Castle::BattalionProductionInfo &info,
		const boost::shared_ptr<MySql::Center_CastleBattalionProduction> &obj)
	{
		PROFILE_ME;

		info.building_base_id      = BuildingBaseId(obj->get_building_base_id());
		info.map_object_type_id    = MapObjectTypeId(obj->get_map_object_type_id());
		info.count                 = obj->get_count();
		info.production_duration   = obj->get_production_duration();
		info.production_time_begin = obj->get_production_time_begin();
		info.production_time_end   = obj->get_production_time_end();
	}

	void fill_building_message(Msg::SC_CastleBuildingBase &msg, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj,
		std::uint64_t utc_now)
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
	void fill_tech_message(Msg::SC_CastleTech &msg, const boost::shared_ptr<MySql::Center_CastleTech> &obj,
		std::uint64_t utc_now)
	{
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
	void fill_battalion_message(Msg::SC_CastleBattalion &msg, const boost::shared_ptr<MySql::Center_CastleBattalion> &obj){
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.map_object_type_id     = obj->get_map_object_type_id();
		msg.count                  = obj->get_count();
		msg.enabled                = obj->get_enabled();
	}
	void fill_battalion_production_message(Msg::SC_CastleBattalionProduction &msg,
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

	bool check_building_mission(const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj, AccountUuid owner_uuid, std::uint64_t utc_now){
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

		async_recheck_building_level_tasks(owner_uuid);

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
}

Castle::Castle(MapObjectUuid map_object_uuid,
	AccountUuid owner_uuid, MapObjectUuid parent_object_uuid, std::string name, Coord coord, std::uint64_t created_time)
	: MapObject(map_object_uuid, MapObjectTypeIds::ID_CASTLE,
		owner_uuid, parent_object_uuid, std::move(name), coord, created_time, false)
{
}
Castle::Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
	const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
	const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBattalion>> &battalions,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBattalionProduction>> &battalion_production)
	: MapObject(std::move(obj), attributes)
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
	for(auto it = battalions.begin(); it != battalions.end(); ++it){
		const auto &obj = *it;
		m_battalions.emplace(MapObjectTypeId(obj->get_map_object_type_id()), obj);
	}
	for(auto it = battalion_production.begin(); it != battalion_production.end(); ++it){
		const auto &obj = *it;
		const auto building_base_id = BuildingBaseId(obj->get_building_base_id());
		if(!building_base_id){
			m_population_production_stamps = obj;
		} else {
			m_battalion_production.emplace(building_base_id, obj);
		}
	}
}
Castle::~Castle(){
}

void Castle::pump_status(){
	PROFILE_ME;

	MapObject::pump_status();

	pump_production();

	const auto utc_now = Poseidon::get_utc_time();

	bool dirty = false;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		if(check_building_mission(it->second, get_owner_uuid(), utc_now)){
			++dirty;
		}
	}
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		if(check_tech_mission(it->second, utc_now)){
			++dirty;
		}
	}
	if(dirty){
		recalculate_attributes();
	}
}
void Castle::recalculate_attributes(){
	PROFILE_ME;

	MapObject::recalculate_attributes();

	std::vector<boost::shared_ptr<MapCell>> map_cells;
	for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
		const auto &map_cell = *it;
		map_cell->pump_status();
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
		const auto building_data = Data::CastleBuilding::require(building_id);
		const auto upgrade_data = Data::CastleUpgradeAbstract::get(building_data->type, building_level);
		if(!upgrade_data){
			continue;
		}

		auto &prosperity = modifiers[AttributeIds::ID_PROSPERITY_POINTS];
		prosperity += static_cast<std::int64_t>(upgrade_data->prosperity_points);

		if(building_data->type == BuildingTypeIds::ID_PRIMARY){
			auto &castle_level = modifiers[AttributeIds::ID_CASTLE_LEVEL];
			if(castle_level < building_level){
				castle_level = building_level;
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

	set_attributes(std::move(modifiers));
}

void Castle::pump_production(){
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
	// production_time_end 是上次人口产出的时间；

	// 人口消耗。
	boost::container::flat_map<ResourceId, std::uint64_t> resources_to_consume_per_minute;
	for(auto it = m_battalions.begin(); it != m_battalions.end(); ++it){
		const auto &obj = it->second;
		const auto soldier_count = obj->get_count();
		if(soldier_count == 0){
			continue;
		}
		const auto map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
		const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
		for(auto rit = map_object_type_data->maintenance_cost.begin(); rit != map_object_type_data->maintenance_cost.end(); ++rit){
			auto &amount_total = resources_to_consume_per_minute[rit->first];
			amount_total = saturated_add(amount_total, saturated_mul(soldier_count, rit->second));
		}
	}
	std::vector<boost::shared_ptr<MapObject>> child_objects;
	WorldMap::get_map_objects_by_parent_object(child_objects, get_map_object_uuid());
	for(auto it = child_objects.begin(); it != child_objects.end(); ++it){
		const auto &child_object = *it;
		const auto map_object_type_id = child_object->get_map_object_type_id();
		if(map_object_type_id == MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto soldier_count = static_cast<std::uint64_t>(child_object->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
		if(soldier_count == 0){
			continue;
		}
		const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
		for(auto rit = map_object_type_data->maintenance_cost.begin(); rit != map_object_type_data->maintenance_cost.end(); ++rit){
			auto &amount_total = resources_to_consume_per_minute[rit->first];
			amount_total = saturated_add(amount_total, saturated_mul(soldier_count, rit->second));
		}
	}
	const auto last_consumption_time = m_population_production_stamps->get_production_time_begin();
	const auto consumption_minutes = saturated_sub(utc_now, last_consumption_time) / 60000;
	const auto consumption_duration = consumption_minutes * 60000;
	if(consumption_minutes > 0){
		LOG_EMPERY_CENTER_DEBUG("Checking population consumption: map_object_uuid = ", get_map_object_uuid(),
			", consumption_minutes = ", consumption_minutes);
		std::vector<ResourceTransactionElement> transaction;
		transaction.reserve(resources_to_consume_per_minute.size());
		for(auto it = resources_to_consume_per_minute.begin(); it != resources_to_consume_per_minute.end(); ++it){
			transaction.emplace_back(ResourceTransactionElement::OP_REMOVE_SATURATED, it->first, saturated_mul(it->second, consumption_minutes),
				ReasonIds::ID_POPULATION_CONSUMPTION, consumption_duration, 0, 0);
		}
		commit_resource_transaction(transaction);
	}
	m_population_production_stamps->set_production_time_begin(last_consumption_time + consumption_duration);

	// 人口产出。
	double production_rate = 0;
	double capacity        = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
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
	if(production_rate > 0){
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
		LOG_EMPERY_CENTER_DEBUG("Checking population production: map_object_uuid = ", get_map_object_uuid(),
			", production_rate = ", production_rate, ", capacity = ", capacity);

		const auto old_last_production_time = m_population_production_stamps->get_production_time_end();
		const auto old_resource_amount      = get_resource(ResourceIds::ID_POPULATION).amount;

		const auto production_duration = saturated_sub(utc_now, old_last_production_time);
		const auto amount_produced = std::round(production_duration * production_rate / 60000.0) + m_production_remainder;
		const auto rounded_amount_produced = static_cast<std::uint64_t>(amount_produced);
		const auto new_resource_amount = std::min<std::uint64_t>(saturated_add(old_resource_amount, rounded_amount_produced), capacity);
		if(new_resource_amount > old_resource_amount){
			std::vector<ResourceTransactionElement> transaction;
			transaction.emplace_back(ResourceTransactionElement::OP_ADD, ResourceIds::ID_POPULATION, new_resource_amount - old_resource_amount,
				ReasonIds::ID_POPULATION_PRODUCTION, production_duration, 0, 0);
			commit_resource_transaction(transaction);

			m_production_remainder = amount_produced - rounded_amount_produced;
		}
	} else {
		// 清空人口？
	}
	m_population_production_stamps->set_production_time_end(utc_now);

	m_production_rate = production_rate;
	m_capacity        = capacity;
}

void Castle::check_init_buildings(){
	PROFILE_ME;

	LOG_EMPERY_CENTER_DEBUG("Checking for init buildings: map_object_uuid = ", get_map_object_uuid());
	bool dirty = false;
	std::vector<boost::shared_ptr<const Data::CastleBuildingBase>> init_buildings;
	Data::CastleBuildingBase::get_init(init_buildings);
	for(auto dit = init_buildings.begin(); dit != init_buildings.end(); ++dit){
		const auto &building_data = *dit;
		const auto building_base_id = building_data->building_base_id;
		const auto bit = m_buildings.find(building_base_id);
		if(bit != m_buildings.end()){
			continue;
		}

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
		if(random_buildings.empty()){
			continue;
		}
		const auto index = static_cast<std::ptrdiff_t>(Poseidon::rand32() % random_buildings.size());
		it = random_buildings.begin() + index;
		const auto building_id = it->first;
		const auto init_level = building_data->init_level;

		LOG_EMPERY_CENTER_DEBUG("> Creating init building: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id, ", building_id = ", building_id);
		auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
			get_map_object_uuid().get(), building_base_id.get(), building_id.get(), init_level, Castle::MIS_NONE, 0, 0, 0);
		obj->async_save(true);
		m_buildings.emplace(building_base_id, std::move(obj));

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
		const auto building_data = Data::CastleBuilding::require(current_id);
		if(building_data->type != type){
			continue;
		}
		BuildingBaseInfo info;
		fill_building_base_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::create_building_mission(BuildingBaseId building_base_id, Castle::Mission mission, BuildingId building_id){
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

	std::uint64_t duration;
	switch(mission){
	case MIS_CONSTRUCT:
		{
			const auto building_data = Data::CastleBuilding::require(building_id);
			const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, 1);
			duration = std::ceil(upgrade_data->upgrade_duration * 60000.0 - 0.001);
		}
		obj->set_building_id(building_id.get());
		obj->set_building_level(0);
		break;

	case MIS_UPGRADE:
		{
			const unsigned level = obj->get_building_level();
			const auto building_data = Data::CastleBuilding::require(BuildingId(obj->get_building_id()));
			const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, level + 1);
			duration = std::ceil(upgrade_data->upgrade_duration * 60000.0 - 0.001);
		}
		break;

	case MIS_DESTRUCT:
		{
			const unsigned level = obj->get_building_level();
			const auto building_data = Data::CastleBuilding::require(BuildingId(obj->get_building_id()));
			const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, level);
			duration = std::ceil(upgrade_data->destruct_duration * 60000.0 - 0.001);
		}
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown building mission: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id, ", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown building mission"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(mission);
	obj->set_mission_duration(duration);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(saturated_add(utc_now, duration));

	if(check_building_mission(obj, get_owner_uuid(), utc_now)){
		recalculate_attributes();
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
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(utc_now);

	// check_building_mission(obj, get_owner_uuid(), utc_now);

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

	if(check_building_mission(obj, get_owner_uuid(), utc_now)){
		recalculate_attributes();
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

void Castle::pump_building_status(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	pump_production();

	const auto utc_now = Poseidon::get_utc_time();

	if(check_building_mission(it->second, get_owner_uuid(), utc_now)){
		recalculate_attributes();
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
bool Castle::is_battalion_production_in_progress(BuildingBaseId building_base_id) const {
	PROFILE_ME;

	const auto it = m_battalion_production.find(building_base_id);
	if(it != m_battalion_production.end()){
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

void Castle::create_tech_mission(TechId tech_id, Castle::Mission mission){
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

	std::uint64_t duration;
	switch(mission){
/*
	case MIS_CONSTRUCT:
		break;
*/
	case MIS_UPGRADE:
		{
			const unsigned level = obj->get_tech_level();
			const auto tech_data = Data::CastleTech::require(TechId(obj->get_tech_id()), level + 1);
			duration = std::ceil(tech_data->upgrade_duration * 60000.0 - 0.001);
		}
		break;
/*
	case MIS_DESTRUCT:
		break;
*/
	default:
		LOG_EMPERY_CENTER_ERROR("Unknown tech mission: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id,
			", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown tech mission"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(mission);
	obj->set_mission_duration(duration);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(saturated_add(utc_now, duration));

	if(check_tech_mission(obj, utc_now)){
		recalculate_attributes();
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
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(utc_now);

	// check_tech_mission(obj, utc_now);

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
		recalculate_attributes();
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

void Castle::pump_tech_status(TechId tech_id){
	PROFILE_ME;

	const auto it = m_techs.find(tech_id);
	if(it == m_techs.end()){
		LOG_EMPERY_CENTER_DEBUG("Tech not found: map_object_uuid = ", get_map_object_uuid(), ", tech_id = ", tech_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	if(check_tech_mission(it->second, utc_now)){
		recalculate_attributes();
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

	LOG_EMPERY_CENTER_DEBUG("Checking for init resources: map_object_uuid = ", get_map_object_uuid());
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
	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleResource>, std::uint64_t /* new_count */> temp_result_map;
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
						obj = boost::make_shared<MySql::Center_CastleResource>(
							get_map_object_uuid().get(), resource_id.get(), 0);
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

Castle::BattalionInfo Castle::get_battalion(MapObjectTypeId map_object_type_id) const {
	PROFILE_ME;

	BattalionInfo info = { };
	info.map_object_type_id = map_object_type_id;

	const auto it = m_battalions.find(map_object_type_id);
	if(it == m_battalions.end()){
		return info;
	}
	fill_battalion_info(info, it->second);
	return info;
}
void Castle::get_all_battalions(std::vector<Castle::BattalionInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_battalions.size());
	for(auto it = m_battalions.begin(); it != m_battalions.end(); ++it){
		BattalionInfo info;
		fill_battalion_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}
void Castle::enable_battalion(MapObjectTypeId map_object_type_id){
	PROFILE_ME;

	auto it = m_battalions.find(map_object_type_id);
	if(it == m_battalions.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBattalion>(
			get_map_object_uuid().get(), map_object_type_id.get(), 0, false);
		obj->async_save(true);
		it = m_battalions.emplace(map_object_type_id, std::move(obj)).first;
	}
	if(it->second->get_enabled()){
		LOG_EMPERY_CENTER_DEBUG("Battalion is already enabled: map_object_uuid = ", get_map_object_uuid().get(),
			", map_object_type_id = ", map_object_type_id);
		return;
	}

	it->second->set_enabled(true);

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBattalion msg;
			fill_battalion_message(msg, it->second);
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
					const auto it = m_battalions.find(map_object_type_id);
					if(it == m_battalions.end()){
						obj = boost::make_shared<MySql::Center_CastleBattalion>(
							get_map_object_uuid().get(), map_object_type_id.get(), 0, false);
						obj->enable_auto_saving(); // obj->async_save(true);
						m_battalions.emplace(map_object_type_id, obj);
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

				LOG_EMPERY_CENTER_DEBUG("@ Battalion transaction: add: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", map_object_type_id = ", map_object_type_id,
					", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::BattalionChanged>(
					map_object_uuid, owner_uuid, map_object_type_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		case SoldierTransactionElement::OP_REMOVE:
		case SoldierTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_battalions.find(map_object_type_id);
				if(it == m_battalions.end()){
					if(operation != SoldierTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Battalion not found: map_object_type_id = ", map_object_type_id);
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
						LOG_EMPERY_CENTER_DEBUG("No enough battalions: map_object_type_id = ", map_object_type_id,
							", temp_count = ", temp_it->second, ", delta_count = ", delta_count);
						return map_object_type_id;
					}
					temp_it->second = 0;
				}
				const auto new_count = temp_it->second;

				LOG_EMPERY_CENTER_DEBUG("@ Battalion transaction: remove: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", map_object_type_id = ", map_object_type_id,
					", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::BattalionChanged>(
					map_object_uuid, owner_uuid, map_object_type_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown battalion transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown battalion transaction operation"));
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
				Msg::SC_CastleBattalion msg;
				fill_battalion_message(msg, it->first);
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
		LOG_EMPERY_CENTER_DEBUG("Insufficient battalions in castle: map_object_uuid = ", get_map_object_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient battalions in castle"));
	}
}

Castle::BattalionProductionInfo Castle::get_battalion_production(BuildingBaseId building_base_id) const {
	PROFILE_ME;

	BattalionProductionInfo info = { };
	info.building_base_id = building_base_id;

	const auto it = m_battalion_production.find(building_base_id);
	if(it == m_battalion_production.end()){
		return info;
	}
	fill_battalion_production_info(info, it->second);
	return info;
}
void Castle::get_all_battalion_production(std::vector<Castle::BattalionProductionInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_battalion_production.size());
	for(auto it = m_battalion_production.begin(); it != m_battalion_production.end(); ++it){
		BattalionProductionInfo info;
		fill_battalion_production_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void Castle::begin_battalion_production(BuildingBaseId building_base_id, MapObjectTypeId map_object_type_id, std::uint64_t count){
	PROFILE_ME;

	auto it = m_battalion_production.find(building_base_id);
	if(it == m_battalion_production.end()){
		auto obj = boost::make_shared<MySql::Center_CastleBattalionProduction>(
			get_map_object_uuid().get(), building_base_id.get(), 0, 0, 0, 0, 0);
		obj->async_save(true);
		it = m_battalion_production.emplace(building_base_id, obj).first;
	}
	const auto &obj = it->second;
	const auto old_map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(old_map_object_type_id != MapObjectTypeId()){
		LOG_EMPERY_CENTER_DEBUG("Battalion production conflict: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Battalion production conflict"));
	}

	const auto map_object_type_data = Data::MapObjectTypeBattalion::require(map_object_type_id);
	const std::uint64_t duration = std::ceil(map_object_type_data->production_time * 60000.0 * count - 0.001);

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_map_object_type_id(map_object_type_id.get());
	obj->set_count(count);
	obj->set_production_duration(duration);
	obj->set_production_time_begin(utc_now);
	obj->set_production_time_end(saturated_add(utc_now, duration));

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBattalionProduction msg;
			fill_battalion_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::cancel_battalion_production(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_battalion_production.find(building_base_id);
	if(it == m_battalion_production.end()){
		LOG_EMPERY_CENTER_DEBUG("Battalion production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}
	const auto &obj = it->second;
	const auto old_map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(old_map_object_type_id == MapObjectTypeId()){
		LOG_EMPERY_CENTER_DEBUG("No battalion production not found: map_object_uuid = ", get_map_object_uuid(),
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
			Msg::SC_CastleBattalionProduction msg;
			fill_battalion_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void Castle::speed_up_battalion_production(BuildingBaseId building_base_id, std::uint64_t delta_duration){
	PROFILE_ME;

	const auto it = m_battalion_production.find(building_base_id);
	if(it == m_battalion_production.end()){
		LOG_EMPERY_CENTER_WARNING("Battalion production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Battalion production not found"));
	}
	const auto &obj = it->second;
	const auto old_map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(old_map_object_type_id == MapObjectTypeId()){
		LOG_EMPERY_CENTER_WARNING("No battalion production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Battalion production not found"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_production_time_end(saturated_sub(obj->get_production_time_end(), delta_duration));

	const auto session = PlayerSessionMap::get(get_owner_uuid());
	if(session){
		try {
			Msg::SC_CastleBattalionProduction msg;
			fill_battalion_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

std::uint64_t Castle::harvest_battalion(BuildingBaseId building_base_id){
	PROFILE_ME;

	const auto it = m_battalion_production.find(building_base_id);
	if(it == m_battalion_production.end()){
		LOG_EMPERY_CENTER_WARNING("Battalion production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Battalion production not found"));
	}
	const auto &obj = it->second;
	const auto map_object_type_id = MapObjectTypeId(obj->get_map_object_type_id());
	if(map_object_type_id == MapObjectTypeId()){
		LOG_EMPERY_CENTER_WARNING("No battalion production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Battalion production not found"));
	}
	const auto count = obj->get_count();

	const auto utc_now = Poseidon::get_utc_time();

	if(utc_now < obj->get_production_time_end()){
		LOG_EMPERY_CENTER_WARNING("Battalion production incomplete: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Battalion production incomplete"));
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
			Msg::SC_CastleBattalionProduction msg;
			fill_battalion_production_message(msg, it->second, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return count;
}

void Castle::synchronize_battalion_production_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto it = m_battalion_production.find(building_base_id);
	if(it == m_battalion_production.end()){
		LOG_EMPERY_CENTER_DEBUG("Battalion production not found: map_object_uuid = ", get_map_object_uuid(),
			", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_CastleBattalionProduction msg;
	fill_battalion_production_message(msg, it->second, utc_now);
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
	for(auto it = m_battalions.begin(); it != m_battalions.end(); ++it){
		Msg::SC_CastleBattalion msg;
		fill_battalion_message(msg, it->second);
		session->send(msg);
	}
	for(auto it = m_battalion_production.begin(); it != m_battalion_production.end(); ++it){
		Msg::SC_CastleBattalionProduction msg;
		fill_battalion_production_message(msg, it->second, utc_now);
		session->send(msg);
	}
}

}
