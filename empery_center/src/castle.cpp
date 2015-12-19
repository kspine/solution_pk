#include "precompiled.hpp"
#include "castle.hpp"
#include "transaction_element.hpp"
#include "map_object.hpp"
#include "map_object_type_ids.hpp"
#include "mysql/castle.hpp"
#include "msg/sc_castle.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/castle.hpp"
#include "building_ids.hpp"
#include "events/resource.hpp"

namespace EmperyCenter {

namespace {
	class FlagGuard {
	private:
		bool &m_locked;

	public:
		explicit FlagGuard(bool &locked)
			: m_locked(locked)
		{
			if(m_locked){
				DEBUG_THROW(Exception, sslit("Transaction locked"));
			}
			m_locked = true;
		}
		~FlagGuard(){
			if(!m_locked){
				assert(false);
			}
			m_locked = false;
		}

		FlagGuard(const FlagGuard &) = delete;
	};

	void fill_building_base_info(Castle::BuildingBaseInfo &info, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj){
		PROFILE_ME;

		info.building_base_id   = BuildingBaseId(obj->get_building_base_id());
		info.building_id        = BuildingId(obj->get_building_id());
		info.building_level     = obj->get_building_level();
		info.mission            = Castle::Mission(obj->get_mission());
		info.mission_duration   = obj->get_mission_duration();
		info.mission_time_begin = obj->get_mission_time_begin();
		info.mission_time_end   = obj->get_mission_time_end();
	}
	void fill_tech_info(Castle::TechInfo &info, const boost::shared_ptr<MySql::Center_CastleTech> &obj){
		PROFILE_ME;

		info.tech_id            = TechId(obj->get_tech_id());
		info.tech_level         = obj->get_tech_level();
		info.mission            = Castle::Mission(obj->get_mission());
		info.mission_duration   = obj->get_mission_duration();
		info.mission_time_begin = obj->get_mission_time_begin();
		info.mission_time_end   = obj->get_mission_time_end();
	}
	void fill_resource_info(Castle::ResourceInfo &info, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		info.resource_id        = ResourceId(obj->get_resource_id());
		info.amount             = obj->get_amount();
	}

	void fill_building_message(Msg::SC_CastleBuildingBase &msg, const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj,
		boost::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.building_base_id       = obj->get_building_base_id();
		msg.building_id            = obj->get_building_id();
		msg.building_level         = obj->get_building_level();
		msg.mission                = obj->get_mission();
		msg.mission_duration       = obj->get_mission_duration();
		// msg.reserved
		msg.mission_time_begin     = obj->get_mission_time_begin();
		msg.mission_time_remaining = saturated_sub(obj->get_mission_time_end(), utc_now);
	}
	void fill_tech_message(Msg::SC_CastleTech &msg, const boost::shared_ptr<MySql::Center_CastleTech> &obj,
		boost::uint64_t utc_now)
	{
		PROFILE_ME;

		msg.map_object_uuid        = obj->unlocked_get_map_object_uuid().to_string();
		msg.tech_id                = obj->get_tech_id();
		msg.tech_level             = obj->get_tech_level();
		msg.mission                = obj->get_mission();
		msg.mission_duration       = obj->get_mission_duration();
		// msg.reserved
		msg.mission_time_begin     = obj->get_mission_time_begin();
		msg.mission_time_remaining = saturated_sub(obj->get_mission_time_end(), utc_now);
	}
	void fill_resource_message(Msg::SC_CastleResource &msg, const boost::shared_ptr<MySql::Center_CastleResource> &obj){
		PROFILE_ME;

		msg.map_object_uuid = obj->unlocked_get_map_object_uuid().to_string();
		msg.resource_id     = obj->get_resource_id();
		msg.amount          = obj->get_amount();
	}

	void check_building_mission(const boost::shared_ptr<MySql::Center_CastleBuildingBase> &obj, boost::uint64_t utc_now){
		PROFILE_ME;

		const auto mission = Castle::Mission(obj->get_mission());
		if(mission == Castle::MIS_NONE){
			return;
		}
		const auto mission_time_end = obj->get_mission_time_end();
		if(utc_now < mission_time_end){
			return;
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
	}
	void check_tech_mission(const boost::shared_ptr<MySql::Center_CastleTech> &obj, boost::uint64_t utc_now){
		PROFILE_ME;

		const auto mission = Castle::Mission(obj->get_mission());
		if(mission == Castle::MIS_NONE){
			return;
		}
		const auto mission_time_end = obj->get_mission_time_end();
		if(utc_now <= mission_time_end){
			return;
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
		obj->set_mission_time_end(utc_now);
	}
}

Castle::Castle(MapObjectUuid map_object_uuid,
	AccountUuid owner_uuid, MapObjectUuid parent_object_uuid, std::string name, Coord coord)
	: MapObject(map_object_uuid, MapObjectTypeIds::ID_CASTLE,
		owner_uuid, parent_object_uuid, std::move(name), coord)
	, m_buildings(
		[&]{
			LOG_EMPERY_CENTER_DEBUG("Checking for init buildings: owner_uuid = ", get_owner_uuid());
			boost::container::flat_map<BuildingBaseId, boost::shared_ptr<MySql::Center_CastleBuildingBase>> buildings;
			std::vector<boost::shared_ptr<const Data::CastleBuildingBase>> init_buildings;
			Data::CastleBuildingBase::get_init(init_buildings);
			for(auto dit = init_buildings.begin(); dit != init_buildings.end(); ++dit){
				const auto &building_data = *dit;
				const auto &buildings_allowed = building_data->buildings_allowed;
				if(buildings_allowed.empty()){
					continue;
				}
				auto it = buildings_allowed.begin();
				it += static_cast<std::ptrdiff_t>(Poseidon::rand32() % buildings_allowed.size());
				const auto building_id = *it;
				const auto init_level = building_data->init_level;

				const auto building_base_id = building_data->building_base_id;
				LOG_EMPERY_CENTER_DEBUG("> Creating init building: map_object_uuid = ", get_map_object_uuid(),
					", building_base_id = ", building_base_id, ", building_id = ", building_id);
				auto obj = boost::make_shared<MySql::Center_CastleBuildingBase>(
					get_map_object_uuid().get(), building_base_id.get(), building_id.get(), init_level, Castle::MIS_NONE, 0, 0, 0);
				obj->async_save(true);
				buildings.emplace(building_base_id, std::move(obj));
			}
			return buildings;
		}())
{
}
Castle::Castle(boost::shared_ptr<MySql::Center_MapObject> obj,
	const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
	const std::vector<boost::shared_ptr<MySql::Center_CastleBuildingBase>> &buildings,
	const std::vector<boost::shared_ptr<MySql::Center_CastleTech>> &techs,
	const std::vector<boost::shared_ptr<MySql::Center_CastleResource>> &resources)
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
}
Castle::~Castle(){
}

void Castle::pump_status(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		check_building_mission(it->second, utc_now);
	}
	for(auto it = m_techs.begin(); it != m_techs.end(); ++it){
		check_tech_mission(it->second, utc_now);
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
std::size_t Castle::count_buildings_by_id(BuildingId building_id) const {
	PROFILE_ME;

	std::size_t count = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		if(BuildingId(it->second->get_building_id()) != building_id){
			continue;
		}
		++count;
	}
	return count;
}
void Castle::get_buildings_by_id(std::vector<Castle::BuildingBaseInfo> &ret, BuildingId building_id) const {
	PROFILE_ME;

	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		if(BuildingId(it->second->get_building_id()) != building_id){
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
		LOG_EMPERY_CENTER_DEBUG("Building mission conflict: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Building mission conflict"));
	}

	boost::uint64_t duration;
	switch(mission){
	case MIS_CONSTRUCT:
		{
			const auto building_data = Data::CastleBuilding::require(building_id);
			const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, 1);
			duration = checked_mul(upgrade_data->upgrade_duration, (boost::uint64_t)60000);
		}
		obj->set_building_id(building_id.get());
		obj->set_building_level(0);
		break;

	case MIS_UPGRADE:
		{
			const unsigned level = obj->get_building_level();
			const auto building_data = Data::CastleBuilding::require(BuildingId(obj->get_building_id()));
			const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, level + 1);
			duration = checked_mul(upgrade_data->upgrade_duration, (boost::uint64_t)60000);
		}
		break;

	case MIS_DESTRUCT:
		{
			const unsigned level = obj->get_building_level();
			const auto building_data = Data::CastleBuilding::require(BuildingId(obj->get_building_id()));
			const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, level);
			duration = checked_mul(upgrade_data->destruct_duration, (boost::uint64_t)60000);
		}
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown building mission: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id,
			", mission = ", (unsigned)mission);
		DEBUG_THROW(Exception, sslit("Unknown building mission"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(mission);
	obj->set_mission_duration(duration);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(saturated_add(utc_now, duration));

	check_building_mission(obj, utc_now);

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
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		return;
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No building mission: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission(MIS_NONE);
	obj->set_mission_duration(0);
	obj->set_mission_time_begin(utc_now);
	obj->set_mission_time_end(utc_now);

	check_building_mission(obj, utc_now);

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
void Castle::speed_up_building_mission(BuildingBaseId building_base_id, boost::uint64_t delta_duration){
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_WARNING("Building base not found: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("Building base not found"));
	}
	const auto &obj = it->second;
	const auto old_mission = Mission(obj->get_mission());
	if(old_mission == MIS_NONE){
		LOG_EMPERY_CENTER_DEBUG("No building mission: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		DEBUG_THROW(Exception, sslit("No building mission"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_mission_time_end(saturated_sub(obj->get_mission_time_end(), delta_duration));

	check_building_mission(obj, utc_now);

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
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	check_building_mission(it->second, utc_now);
}
void Castle::synchronize_building_with_player(BuildingBaseId building_base_id, const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto it = m_buildings.find(building_base_id);
	if(it == m_buildings.end()){
		LOG_EMPERY_CENTER_DEBUG("Building base not found: map_object_uuid = ", get_map_object_uuid(), ", building_base_id = ", building_base_id);
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	Msg::SC_CastleBuildingBase msg;
	fill_building_message(msg, it->second, utc_now);
	session->send(msg);
}

unsigned Castle::get_level() const {
	PROFILE_ME;

	unsigned level = 0;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(building_id != BuildingIds::ID_PRIMARY){
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
boost::container::flat_map<ResourceId, boost::uint64_t> Castle::get_max_resource_amounts() const {
	PROFILE_ME;

	boost::container::flat_map<ResourceId, boost::uint64_t> max_amounts;
	for(auto it = m_buildings.begin(); it != m_buildings.end(); ++it){
		const auto building_id = BuildingId(it->second->get_building_id());
		if(building_id != BuildingIds::ID_WAREHOUSE){
			continue;
		}
		const unsigned current_level = it->second->get_building_level();
		const auto data = Data::CastleUpgradeWarehouse::require(current_level);
		max_amounts.reserve(data->max_resource_amounts.size());
		for(auto it = data->max_resource_amounts.begin(); it != data->max_resource_amounts.end(); ++it){
			auto &val = max_amounts[it->first];
			val = saturated_add(val, it->second);
		}
	}
	return max_amounts;
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

	boost::uint64_t duration;
	switch(mission){
//	case MIS_CONSTRUCT:
//		break;

	case MIS_UPGRADE:
		{
			const unsigned level = obj->get_tech_level();
			const auto tech_data = Data::CastleTech::require(TechId(obj->get_tech_id()), level + 1);
			duration = checked_mul(tech_data->upgrade_duration, (boost::uint64_t)60000);
		}
		break;

//	case MIS_DESTRUCT:
//		break;

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

	check_tech_mission(obj, utc_now);

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

	check_tech_mission(obj, utc_now);

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
void Castle::speed_up_tech_mission(TechId tech_id, boost::uint64_t delta_duration){
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

	check_tech_mission(obj, utc_now);

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

	check_tech_mission(it->second, utc_now);
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

	const FlagGuard transaction_guard(m_locked_by_transaction);

	boost::shared_ptr<bool> withdrawn;
	boost::container::flat_map<boost::shared_ptr<MySql::Center_CastleResource>, boost::uint64_t /* new_count */> temp_result_map;

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

				const auto &map_object_uuid = get_map_object_uuid();
				const auto &owner_uuid = get_owner_uuid();
				LOG_EMPERY_CENTER_DEBUG("@ Resource transaction: add: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", resource_id = ", resource_id, ", old_amount = ", old_amount, ", delta_amount = ", delta_amount, ", new_amount = ", new_amount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::async_raise_event(
					boost::make_shared<Events::ResourceChanged>(map_object_uuid, owner_uuid,
						resource_id, old_amount, new_amount, reason, param1, param2, param3),
					withdrawn);
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

				const auto &map_object_uuid = get_map_object_uuid();
				const auto &owner_uuid = get_owner_uuid();
				LOG_EMPERY_CENTER_DEBUG("@ Resource transaction: remove: map_object_uuid = ", map_object_uuid, ", owner_uuid = ", owner_uuid,
					", resource_id = ", resource_id, ", old_amount = ", old_amount, ", delta_amount = ", delta_amount, ", new_amount = ", new_amount,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				if(!withdrawn){
					withdrawn = boost::make_shared<bool>(true);
				}
				Poseidon::async_raise_event(
					boost::make_shared<Events::ResourceChanged>(map_object_uuid, owner_uuid,
						resource_id, old_amount, new_amount, reason, param1, param2, param3),
					withdrawn);
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown resource transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown resource transaction operation"));
		}
	}

	if(callback){
		callback();
	}

	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_amount(it->second);
	}
	if(withdrawn){
		*withdrawn = false;
	}

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
}

}
