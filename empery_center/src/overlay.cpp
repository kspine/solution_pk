#include "precompiled.hpp"
#include "overlay.hpp"
#include "mysql/overlay.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"
#include "castle.hpp"
#include "checked_arithmetic.hpp"
#include "data/map.hpp"
#include "singletons/account_map.hpp"
#include "data/global.hpp"
#include "data/map_object_type.hpp"
#include "attribute_ids.hpp"

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

std::uint64_t Overlay::harvest(const boost::shared_ptr<MapObject> &harvester, std::uint64_t duration, bool saturated){
	PROFILE_ME;

	const auto parent_object_uuid = harvester->get_parent_object_uuid();
	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
	if(!castle){
		LOG_EMPERY_CENTER_DEBUG("No parent castle: harvester_uuid = ", harvester->get_map_object_uuid(),
			", parent_object_uuid = ", parent_object_uuid);
		return 0;
	}

	const auto cluster_coord = get_cluster_coord();
	const auto &overlay_group_name = get_overlay_group_name();

	const auto resource_id = get_resource_id();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No resource id: overlay_group_name = ", overlay_group_name);
		return 0;
	}

	const auto harvester_type_id = harvester->get_map_object_type_id();
	const auto harvester_type_data = Data::MapObjectTypeBattalion::require(harvester_type_id);
	const auto harvest_speed = harvester_type_data->harvest_speed;
	if(harvest_speed <= 0){
		LOG_EMPERY_CENTER_DEBUG("Harvest speed is zero: harvester_type_id = ", harvester_type_id);
		return 0;
	}
	const auto amount_remaining = get_resource_amount();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		return 0;
	}
	const auto soldier_count = static_cast<std::uint64_t>(std::max<std::int64_t>(harvester->get_attribute(AttributeIds::ID_SOLDIER_COUNT), 0));
	const auto amount_to_harvest = harvest_speed * duration * soldier_count / 60000.0 + m_harvest_remainder;
	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining);

	const auto capacity_remaining = saturated_sub(castle->get_warehouse_capacity(resource_id), castle->get_resource(resource_id).amount);
	const auto amount_to_add = std::min(rounded_amount_removable, capacity_remaining);
	const auto amount_to_remove = saturated ? rounded_amount_removable : amount_to_add;
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name,
		", resource_id = ", resource_id, ", amount_to_add = ", amount_to_add, ", amount_to_remove = ", amount_to_remove);

	std::vector<ResourceTransactionElement> transaction;
	transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add,
		ReasonIds::ID_HARVEST_OVERLAY, cluster_coord.x(), cluster_coord.y(), get_overlay_id().get());
	castle->commit_resource_transaction(transaction,
		[&]{ m_obj->set_resource_amount(checked_sub(m_obj->get_resource_amount(), amount_to_remove)); });

	m_harvest_remainder = amount_to_harvest - rounded_amount_to_harvest;

	WorldMap::update_overlay(virtual_shared_from_this<Overlay>(), false);

	return amount_to_remove;
}

bool Overlay::is_virtually_removed() const {
	return get_resource_amount() == 0;
}
void Overlay::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_MapOverlayRemoved msg;
		msg.cluster_x          = get_cluster_coord().x();
		msg.cluster_y          = get_cluster_coord().y();
		msg.overlay_group_name = get_overlay_group_name();
		session->send(msg);
	} else {
		Msg::SC_MapOverlayInfo msg;
		msg.cluster_x          = get_cluster_coord().x();
		msg.cluster_y          = get_cluster_coord().y();
		msg.overlay_group_name = get_overlay_group_name();
		msg.resource_amount    = get_resource_amount();
		session->send(msg);
	}
}

}
