#include "precompiled.hpp"
#include "overlay.hpp"
#include "mysql/overlay.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "checked_arithmetic.hpp"
#include "data/map.hpp"
#include "singletons/account_map.hpp"
#include "data/global.hpp"
#include "data/map_object_type.hpp"
#include "data/castle.hpp"
#include "attribute_ids.hpp"
#include "map_object.hpp"

namespace EmperyCenter {

Overlay::Overlay(Coord cluster_coord, std::string overlay_group_name,
	OverlayId overlay_id, ResourceId resource_id, std::uint64_t resource_amount)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_Overlay>(cluster_coord.x(), cluster_coord.y(), std::move(overlay_group_name),
				overlay_id.get(), resource_id.get(), resource_amount);
			obj->async_save(true);
			return obj;
		}())
{
}
Overlay::Overlay(boost::shared_ptr<MySql::Center_Overlay> obj)
	: m_obj(std::move(obj))
{
}
Overlay::~Overlay(){
}

void Overlay::pump_status(){
}

Coord Overlay::get_cluster_coord() const {
	return Coord(m_obj->get_cluster_x(), m_obj->get_cluster_y());
}
const std::string &Overlay::get_overlay_group_name() const {
	return m_obj->unlocked_get_overlay_group_name();
}
OverlayId Overlay::get_overlay_id() const {
	return OverlayId(m_obj->get_overlay_id());
}

ResourceId Overlay::get_resource_id() const {
	return ResourceId(m_obj->get_resource_id());
}
std::uint64_t Overlay::get_resource_amount() const {
	return m_obj->get_resource_amount();
}

std::uint64_t Overlay::harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated){
	PROFILE_ME;

	const auto cluster_coord = get_cluster_coord();
	const auto &overlay_group_name = get_overlay_group_name();

	const auto resource_id = get_resource_id();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No resource id: overlay_group_name = ", overlay_group_name);
		return 0;
	}
	const auto resource_data = Data::CastleResource::require(resource_id);
	const auto carried_attribute_id = resource_data->carried_attribute_id;
	if(!carried_attribute_id){
		LOG_EMPERY_CENTER_DEBUG("Resource is not harvestable: resource_id = ", resource_id);
		return 0;
	}
	const auto amount_remaining = get_resource_amount();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		return 0;
	}

	const auto harvester_type_id = harvester->get_map_object_type_id();
	const auto harvester_type_data = Data::MapObjectTypeBattalion::require(harvester_type_id);

	amount_to_harvest += m_harvest_remainder;
	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining);

	const auto soldier_count = static_cast<std::uint64_t>(harvester->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto resource_capacity = static_cast<std::uint64_t>(std::floor(harvester_type_data->resource_carriable * soldier_count + 0.001));
	const auto resource_amount_carried = harvester->get_resource_amount_carried();
	const auto capacity_remaining = saturated_sub(resource_capacity, resource_amount_carried);
	const auto amount_to_add = std::min(rounded_amount_removable, capacity_remaining);
	const auto amount_to_remove = saturated ? rounded_amount_removable : amount_to_add;
	LOG_EMPERY_CENTER_DEBUG("Harvesting overlay: cluster_coord = ", cluster_coord,
		", overlay_group_name = ", overlay_group_name, ", resource_id = ", resource_id, ", carried_attribute_id = ", carried_attribute_id,
		", resource_amount_carried = ", resource_amount_carried, ", capacity_remaining = ", capacity_remaining,
		", amount_to_add = ", amount_to_add, ", amount_to_remove = ", amount_to_remove);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[carried_attribute_id] = harvester->get_attribute(carried_attribute_id) + static_cast<std::int64_t>(amount_to_add);

	harvester->set_attributes(std::move(modifiers));
	m_obj->set_resource_amount(checked_sub(m_obj->get_resource_amount(), amount_to_remove));
	m_harvest_remainder = amount_to_harvest - rounded_amount_to_harvest;
	m_last_harvester = harvester;

	WorldMap::update_overlay(virtual_shared_from_this<Overlay>(), false);

	return amount_to_remove;
}

bool Overlay::is_virtually_removed() const {
	return get_resource_amount() == 0;
}
void Overlay::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	std::vector<boost::shared_ptr<const Data::MapCellBasic>> cells_in_group;
	Data::MapCellBasic::get_by_overlay_group(cells_in_group, get_overlay_group_name());
	if(cells_in_group.empty()){
		LOG_EMPERY_CENTER_ERROR("Overlay group not found: overlay_group_name = ", get_overlay_group_name());
		DEBUG_THROW(Exception, sslit("Overlay group not found"));
	}
	std::uint64_t sum_x = 0, sum_y = 0;
	for(auto it = cells_in_group.begin(); it != cells_in_group.end(); ++it){
		const auto &basic_data = *it;
		sum_x += basic_data->map_coord.first;
		sum_y += basic_data->map_coord.second;
	}
	const auto coord_hint = Coord(get_cluster_coord().x() + static_cast<unsigned>(sum_x / cells_in_group.size()),
	                              get_cluster_coord().y() + static_cast<unsigned>(sum_y / cells_in_group.size()));

	if(is_virtually_removed()){
		Msg::SC_MapOverlayRemoved msg;
		msg.cluster_x          = get_cluster_coord().x();
		msg.cluster_y          = get_cluster_coord().y();
		msg.overlay_group_name = get_overlay_group_name();
		msg.coord_hint_x       = coord_hint.x();
		msg.coord_hint_y       = coord_hint.y();
		session->send(msg);
	} else {
		Msg::SC_MapOverlayInfo msg;
		msg.cluster_x          = get_cluster_coord().x();
		msg.cluster_y          = get_cluster_coord().y();
		msg.overlay_group_name = get_overlay_group_name();
		msg.resource_amount    = get_resource_amount();
		const auto last_harvester = get_last_harvester();
		if(last_harvester){
			msg.last_harvested_account_uuid = last_harvester->get_owner_uuid().str();
			msg.last_harvested_object_uuid  = last_harvester->get_map_object_uuid().str();
		}
		msg.coord_hint_x       = coord_hint.x();
		msg.coord_hint_y       = coord_hint.y();
		session->send(msg);
	}
}

}
