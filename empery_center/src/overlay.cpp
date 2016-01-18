#include "precompiled.hpp"
#include "overlay.hpp"
#include "mysql/overlay.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "cluster_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"
#include "castle.hpp"
#include "checked_arithmetic.hpp"
#include "data/map.hpp"
#include "singletons/account_map.hpp"
#include "data/global.hpp"
#include "data/map_object.hpp"

namespace EmperyCenter {

Overlay::Overlay(Coord cluster_coord, std::string overlay_group_name, OverlayId overlay_id)
	: m_obj(
		[&]{
			const auto overlay_data = Data::MapOverlay::require(overlay_id);

			std::vector<boost::shared_ptr<const Data::MapCellBasic>> cells_in_group;
			Data::MapCellBasic::get_by_overlay_group(cells_in_group, overlay_group_name);
			if(cells_in_group.empty()){
				LOG_EMPERY_CENTER_ERROR("No cells in overlay group? overlay_group_name = ", overlay_group_name);
				DEBUG_THROW(Exception, sslit("No cells in overlay group"));
			}
			std::uint64_t sum_resources = 0;
			for(auto it = cells_in_group.begin(); it != cells_in_group.end(); ++it){
				const auto &basic_data = *it;
				const auto overlay_data = Data::MapOverlay::require(basic_data->overlay_id);
				sum_resources = checked_add(sum_resources, overlay_data->reward_resource_amount);
			}

			auto obj = boost::make_shared<MySql::Center_Overlay>(cluster_coord.x(), cluster_coord.y(), std::move(overlay_group_name),
				overlay_id.get(), overlay_data->reward_resource_id.get(), sum_resources);
			obj->enable_auto_saving(); // obj->async_save(true);
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
	const auto overlay_id = get_overlay_id();

	const auto overlay_data = Data::MapOverlay::require(overlay_id);
	const auto resource_id = overlay_data->reward_resource_id;
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No reward resource id: overlay_group_name = ", overlay_group_name, ", resource_id = ", resource_id);
		return 0;
	}

	const auto harvester_type_id = harvester->get_map_object_type_id();
	const auto harvester_data = Data::MapObject::require(harvester_type_id);
	const auto harvest_speed = harvester_data->harvest_speed;
	if(harvest_speed <= 0){
		LOG_EMPERY_CENTER_DEBUG("Harvest speed is zero: harvester_type_id = ", harvester_type_id);
		return 0;
	}
	const auto amount_to_harvest = harvest_speed * duration / 60000.0 + m_harvest_remainder;
	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto amount_remaining = get_resource_amount();
	const auto amount_avail = std::min(rounded_amount_to_harvest, amount_remaining);
	if(amount_avail == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		return 0;
	}

	const auto capacity_remaining = saturated_sub(castle->get_max_resource_amount(resource_id), castle->get_resource(resource_id).amount);
	const auto amount_to_add = std::min(amount_avail, capacity_remaining);
	const auto amount_to_remove = saturated ? amount_avail : amount_to_add;
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name,
		", resource_id = ", resource_id, ", amount_to_add = ", amount_to_add, ", amount_to_remove = ", amount_to_remove);

	std::vector<ResourceTransactionElement> transaction;
	transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add,
		ReasonIds::ID_HARVEST_OVERLAY, cluster_coord.x(), cluster_coord.y(), overlay_id.get());
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
void Overlay::synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const {
	PROFILE_ME;

	Msg::SK_MapAddOverlay msg;
	msg.cluster_x          = get_cluster_coord().x();
	msg.cluster_y          = get_cluster_coord().y();
	msg.overlay_group_name = get_overlay_group_name();
	msg.overlay_id         = get_overlay_id().get();
	msg.resource_amount    = get_resource_amount();
	cluster->send(msg);
}

}
