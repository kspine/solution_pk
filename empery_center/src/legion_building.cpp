#include "precompiled.hpp"
#include "account.hpp"
#include "mmain.hpp"
#include "mysql/legion.hpp"
#include "legion_building.hpp"
#include "singletons/legion_building_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>


namespace EmperyCenter {

std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<LegionBuilding>> LegionBuilding::async_create(
	LegionBuildingUuid legion_building_uuid,   LegionUuid legion_uuid,  std::uint64_t ntype)
{
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_LegionBuilding>(legion_building_uuid.get(),legion_uuid.get() ,ntype);
	obj->enable_auto_saving();
	auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	auto account = boost::make_shared<LegionBuilding>(std::move(obj), std::vector<boost::shared_ptr<MySql::Center_LegionBuildingAttribute>>());
	return std::make_pair(std::move(promise), std::move(account));
}

LegionBuilding::LegionBuilding(boost::shared_ptr<MySql::Center_LegionBuilding> obj,
	const std::vector<boost::shared_ptr<MySql::Center_LegionBuildingAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(LegionBuildingAttributeId((*it)->get_legion_building_attribute_id()), *it);
	}

}
LegionBuilding::~LegionBuilding(){
}

LegionBuildingUuid LegionBuilding::get_legion_building_uuid()  const {
	return LegionBuildingUuid(m_obj->unlocked_get_legion_building_uuid());
}

LegionUuid LegionBuilding::get_legion_uuid()  const {
	return LegionUuid(m_obj->unlocked_get_legion_uuid());
}

std::uint64_t LegionBuilding::get_type()  const {
	return std::uint64_t(m_obj->unlocked_get_ntype());
}

void LegionBuilding::InitAttributes(std::string strobjuuid)
{
	PROFILE_ME;

	// 设置属性
	boost::container::flat_map<LegionBuildingAttributeId, std::string> modifiers;
	modifiers.emplace(LegionBuildingAttributeIds::ID_LEVEL, "0");
	modifiers.emplace(LegionBuildingAttributeIds::ID_MAPOBJECT_UUID, strobjuuid);

	set_attributes(std::move(modifiers));

}

void LegionBuilding::leave()
{
	// 删除属性表
	std::string strsql = "DELETE FROM Center_LegionBuildingAttribute WHERE legion_building_uuid='";
	strsql += get_legion_building_uuid().str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("Center_LegionBuildingAttribute",strsql);
}


const std::string &LegionBuilding::get_attribute(LegionBuildingAttributeId account_attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(account_attribute_id);
	if(it == m_attributes.end()){
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void LegionBuilding::get_attributes(boost::container::flat_map<LegionBuildingAttributeId, std::string> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->unlocked_get_value();
	}
}
void LegionBuilding::set_attributes(boost::container::flat_map<LegionBuildingAttributeId, std::string> modifiers){
	PROFILE_ME;


	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_LegionBuildingAttribute>(m_obj->get_legion_building_uuid(),
				it->first.get(), std::string());
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));

		LOG_EMPERY_CENTER_DEBUG("LegionBuilding::set_attributes  account_uuid= ", m_obj->get_legion_building_uuid(), " key:",it->first,"  value:",std::move(it->second));
	}

	LegionBuildingMap::update(virtual_shared_from_this<LegionBuilding>(), false);
}

}
