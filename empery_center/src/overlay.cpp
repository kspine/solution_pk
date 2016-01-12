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
#include "data/map.hpp"

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

std::uint64_t Overlay::harvest(const boost::shared_ptr<Castle> &castle, std::uint64_t max_amount, bool saturated){
	PROFILE_ME;

	const auto cluster_coord = get_cluster_coord();
	const auto &overlay_group_name = get_overlay_group_name();
	const auto overlay_id = get_overlay_id();

	const auto overlay_data = Data::MapOverlay::require(overlay_id);
	const auto resource_id = overlay_data->reward_resource_id;
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No reward resource id: overlay_group_name = ", overlay_group_name, ", resource_id = ", resource_id);
		return 0;
	}
	auto amount_avail = get_resource_amount();
	if(amount_avail > max_amount){
		amount_avail = max_amount;
	}
	if(amount_avail == 0){
		LOG_EMPERY_CENTER_DEBUG("Zero amount specified: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		return 0;
	}

	auto amount_to_add = amount_avail;
	const auto max_amount_to_add = saturated_sub(castle->get_max_resource_amount(resource_id), castle->get_resource(resource_id).amount);
	if(amount_to_add > max_amount_to_add){
		amount_to_add = max_amount_to_add;
	}
	auto amount_to_remove = amount_avail;
	if(!saturated){
		amount_to_remove = amount_to_add;
	}
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name,
		", resource_id = ", resource_id, ", amount_to_add = ", amount_to_add, ", amount_to_remove = ", amount_to_remove);

	std::vector<ResourceTransactionElement> transaction;
	transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount_to_add,
		ReasonIds::ID_HARVEST_OVERLAY, cluster_coord.x(), cluster_coord.y(), overlay_id.get());
	castle->commit_resource_transaction(transaction,
		[&]{ m_obj->set_resource_amount(checked_sub(m_obj->get_resource_amount(), amount_to_remove)); });

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
