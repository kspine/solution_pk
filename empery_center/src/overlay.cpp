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
	: m_obj([&]{
		std::vector<boost::shared_ptr<const Data::MapCellBasic>> cells_in_group;
		Data::MapCellBasic::get_by_overlay_group(cells_in_group, overlay_group_name);
		if(cells_in_group.empty()){
			LOG_EMPERY_CENTER_ERROR("No cells in overlay group? overlay_group_name = ", overlay_group_name);
			DEBUG_THROW(Exception, sslit("No cells in overlay group"));
		}
		boost::uint64_t sum_x = 0, sum_y = 0;
		for(auto it = cells_in_group.begin(); it != cells_in_group.end(); ++it){
			const auto &basic_data = *it;
			sum_x = checked_add(sum_x, static_cast<boost::uint64_t>(basic_data->map_coord.first));
			sum_y = checked_add(sum_y, static_cast<boost::uint64_t>(basic_data->map_coord.second));
		}
		const auto x = cluster_coord.x() + static_cast<long>(sum_x / cells_in_group.size());
		const auto y = cluster_coord.y() + static_cast<long>(sum_y / cells_in_group.size());

		const auto overlay_data = Data::MapOverlay::require(overlay_id);

		auto obj = boost::make_shared<MySql::Center_Overlay>(cluster_coord.x(), cluster_coord.y(), std::move(overlay_group_name),
			overlay_id.get(), x, y, overlay_data->reward_resource_id.get(), overlay_data->reward_resource_amount);
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

Coord Overlay::get_cluster_coord() const {
	return Coord(m_obj->get_cluster_x(), m_obj->get_cluster_y());
}
const std::string &Overlay::get_overlay_group_name() const {
	return m_obj->unlocked_get_overlay_group_name();
}
OverlayId Overlay::get_overlay_id() const {
	return OverlayId(m_obj->get_overlay_id());
}

Coord Overlay::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}
ResourceId Overlay::get_resource_id() const {
	return ResourceId(m_obj->get_resource_id());
}
boost::uint64_t Overlay::get_resource_amount() const {
	return m_obj->get_resource_amount();
}

boost::uint64_t Overlay::harvest(const boost::shared_ptr<Castle> &castle, boost::uint64_t max_amount){
	PROFILE_ME;

	const auto cluster_coord = get_cluster_coord();
	const auto &overlay_group_name = get_overlay_group_name();
	const auto overlay_id = get_overlay_id();
	const auto amount = std::min(get_resource_amount(), max_amount);
	if(amount == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource can be harvested: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name);
		return 0;
	}
	const auto overlay_data = Data::MapOverlay::require(overlay_id);
	const auto resource_id = overlay_data->reward_resource_id;
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource: cluster_coord = ", cluster_coord, ", overlay_group_name = ", overlay_group_name,
		", resource_id = ", resource_id, ", amount = ", amount);

	std::vector<ResourceTransactionElement> transaction;
	transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount,
		ReasonIds::ID_HARVEST_OVERLAY, cluster_coord.x(), cluster_coord.y(), overlay_id.get());
	castle->commit_resource_transaction(transaction,
		[&]{ m_obj->set_resource_amount(checked_sub(m_obj->get_resource_amount(), amount)); });
	return amount;
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
		msg.overlay_id         = get_overlay_id().get();
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
