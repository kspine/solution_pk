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

namespace EmperyCenter {

Overlay::Overlay(Coord cluster_coord, std::string overlay_group, OverlayId overlay_id, boost::uint64_t resource_amount)
	: m_obj([&]{
		auto obj = boost::make_shared<MySql::Center_Overlay>(cluster_coord.x(), cluster_coord.y(),
			std::move(overlay_group), overlay_id.get(), resource_amount);
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
const std::string &Overlay::get_overlay_group() const {
	return m_obj->unlocked_get_overlay_group();
}
OverlayId Overlay::get_overlay_id() const {
	return OverlayId(m_obj->get_overlay_id());
}

std::string Overlay::get_overlay_unique_name() const {
	PROFILE_ME;

	std::ostringstream oss;
	oss <<get_cluster_coord() <<',' <<get_overlay_group();
	return oss.str();
}

boost::uint64_t Overlay::get_resource_amount() const {
	return m_obj->get_resource_amount();
}

boost::uint64_t Overlay::harvest(const boost::shared_ptr<Castle> &castle, boost::uint64_t max_amount){
	PROFILE_ME;

	const auto cluster_coord = get_cluster_coord();
	const auto &overlay_group = get_overlay_group();
	const auto overlay_id = get_overlay_id();
	const auto amount = std::min(get_resource_amount(), max_amount);
	if(amount == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource can be harvested: cluster_coord = ", cluster_coord, ", overlay_group = ", overlay_group);
		return 0;
	}
	const auto overlay_data = Data::MapOverlay::require(overlay_id);
	const auto resource_id = overlay_data->reward_resource_id;
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource: cluster_coord = ", cluster_coord, ", overlay_group = ", overlay_group,
		", resource_id = ", resource_id, ", amount = ", amount);

	std::vector<ResourceTransactionElement> transaction;
	transaction.emplace_back(ResourceTransactionElement::OP_ADD, resource_id, amount,
		ReasonIds::ID_HARVEST_OVERLAY, cluster_coord.x(), cluster_coord.y(), overlay_id.get());
	castle->commit_resource_transaction(transaction,
		[&]{ m_obj->set_resource_amount(checked_sub(m_obj->get_resource_amount(), amount)); });
	return amount;
}

void Overlay::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto resource_amount = get_resource_amount();
	if(resource_amount == 0){
		Msg::SC_MapOverlayRemoved msg;
		msg.overlay_unique_name = get_overlay_unique_name();
		session->send(msg);
	} else {
		Msg::SC_MapOverlayInfo msg;
		msg.overlay_unique_name = get_overlay_unique_name();
		msg.overlay_id          = get_overlay_id().get();
		msg.resource_amount     = get_resource_amount();
		session->send(msg);
	}
}
void Overlay::synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const {
	PROFILE_ME;

	const auto resource_amount = get_resource_amount();
	if(resource_amount == 0){
		Msg::SK_MapRemoveOverlay msg;
		msg.overlay_unique_name = get_overlay_unique_name();
		cluster->send(msg);
	} else {
		Msg::SK_MapAddOverlay msg;
		msg.overlay_unique_name = get_overlay_unique_name();
		msg.overlay_id          = get_overlay_id().get();
		msg.resource_amount     = get_resource_amount();
		cluster->send(msg);
	}
}

}
