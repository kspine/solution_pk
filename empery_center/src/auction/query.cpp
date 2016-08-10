#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../auction_center.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/global.hpp"
#include "../data/item.hpp"
#include "../data/castle.hpp"
#include "../map_object_type_ids.hpp"
#include "../singletons/world_map.hpp"
#include "../data/map.hpp"
#include "../resource_ids.hpp"
#include "../map_cell.hpp"

namespace EmperyCenter {

AUCTION_SERVLET("query/account", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");

	const auto account = AccountMap::get_or_reload_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto account_uuid = account->get_account_uuid();

	AccountMap::require_controller_token(account_uuid, { });

	const auto auction_center = AuctionCenterMap::require(account_uuid);
	const auto item_box = ItemBoxMap::require(account_uuid);
	auction_center->pump_status();
	item_box->pump_status();

	// 获取账号状态。
	{
		Poseidon::JsonObject elem_account;

		elem_account[sslit("account_uuid")]    = account->get_account_uuid().str();
		elem_account[sslit("promotion_level")] = account->get_promotion_level();
		elem_account[sslit("nick")]            = account->get_nick();

		root[sslit("account")] = std::move(elem_account);
	}

	// 获取交易中心状态。
	{
		Poseidon::JsonObject elem_auction_center;

		{
			std::vector<AuctionCenter::TransferInfo> transfers;
			auction_center->get_transfers_all(transfers);

			boost::container::flat_map<MapObjectUuid, Poseidon::JsonObject> castle_map;
			castle_map.reserve(transfers.size());
			for(auto it = transfers.begin(); it != transfers.end(); ++it){
				Poseidon::JsonObject elem_castle;
				elem_castle[sslit("item_count")]             = it->item_count;
				elem_castle[sslit("item_count_locked")]      = it->item_count_locked;
				elem_castle[sslit("item_count_fee")]         = it->item_count_fee;
				elem_castle[sslit("resource_id")]            = it->resource_id.get();
				elem_castle[sslit("resource_amount_locked")] = it->resource_amount_locked;
				elem_castle[sslit("resource_amount_fee")]    = it->resource_amount_fee;
				elem_castle[sslit("created_time")]           = it->created_time;
				elem_castle[sslit("due_time")]               = it->due_time;

				char str[64];
				unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->item_id.get());
				castle_map[it->map_object_uuid][SharedNts(str, len)] = std::move(elem_castle);
			}

			Poseidon::JsonObject elem_castles;
			for(auto it = castle_map.begin(); it != castle_map.end(); ++it){
				const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(it->first));
				if(!castle){
					continue;
				}

				Poseidon::JsonObject elem_castle;
				elem_castle[sslit("resources")] = std::move(it->second);
				elem_castle[sslit("name")] = castle->get_name();

				elem_castles[boost::lexical_cast<SharedNts>(it->first)] = std::move(elem_castle);
			}
			elem_auction_center[sslit("castles")] = std::move(elem_castles);
		}

		{
			std::vector<AuctionCenter::ItemInfo> items;
			auction_center->get_items_all(items);

			Poseidon::JsonObject elem_items;
			for(auto it = items.begin(); it != items.end(); ++it){
				char str[64];
				unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->item_id.get());
				elem_items[SharedNts(str, len)] = it->count;
			}
			elem_auction_center[sslit("items")] = std::move(elem_items);
		}

		root[sslit("auction_center")] = std::move(elem_auction_center);
	}

	// 获取城堡状态。
	{
		Poseidon::JsonObject elem_castles;

		std::vector<boost::shared_ptr<MapObject>> map_objects;
		WorldMap::get_map_objects_by_owner(map_objects, account->get_account_uuid());
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto &map_object = *it;
			if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
				continue;
			}
			const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
			if(!castle){
				continue;
			}
			castle->pump_status();

			Poseidon::JsonObject elem_castle;
			{
				Poseidon::JsonObject elem_resources;

				std::vector<Castle::ResourceInfo> resource_info_all;
				castle->get_resources_all(resource_info_all);
				for(auto it = resource_info_all.begin(); it != resource_info_all.end(); ++it){
					const auto resource_id = it->resource_id;
					const auto amount      = it->amount;

					const auto resource_data = Data::CastleResource::get(resource_id);
					if(!resource_data){
						LOG_EMPERY_CENTER_DEBUG("Resource data not found: castle_uuid = ", castle->get_map_object_uuid(),
							", owner_uuid = ", castle->get_owner_uuid(), ", resource_id = ", resource_id);
						continue;
					}
					if(!resource_data->locked_resource_id){
						continue;
					}
					char str[64];
					unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)resource_data->undeployed_item_id.get());
					elem_resources[SharedNts(str, len)] = amount;
				}

				elem_castle[sslit("resources")] = std::move(elem_resources);
			}
			elem_castle[sslit("name")] = castle->get_name();
			elem_castle[sslit("level")] = castle->get_level();

			boost::container::flat_map<ResourceId, std::uint64_t> protection_cost_total;
			protection_cost_total.reserve(4);
			{
				const auto castle_level = castle->get_level();
				const auto upgrade_data = Data::CastleUpgradePrimary::require(castle_level);
				const auto &protection_cost = upgrade_data->protection_cost;
				for(auto it = protection_cost.begin(); it != protection_cost.end(); ++it){
					auto &amount_total = protection_cost_total[it->first];
					amount_total = checked_add(amount_total, it->second);
				}

				std::vector<boost::shared_ptr<MapCell>> map_cells;
				WorldMap::get_map_cells_by_parent_object(map_cells, castle->get_map_object_uuid());
				for(auto it = map_cells.begin(); it != map_cells.end(); ++it){
					const auto &map_cell = *it;
					if(!map_cell->is_protectable()){
						continue;
					}
					const auto coord = map_cell->get_coord();
					const auto cluster_scope = WorldMap::get_cluster_scope(coord);
					const auto basic_data = Data::MapCellBasic::require(static_cast<unsigned>(coord.x() - cluster_scope.left()),
				                                                    	static_cast<unsigned>(coord.y() - cluster_scope.bottom()));
					const auto terrain_data = Data::MapTerrain::require(basic_data->terrain_id);
					auto &amount_total = protection_cost_total[ResourceIds::ID_SPRING_WATER];
					amount_total = checked_add(amount_total, terrain_data->protection_cost);
				}
			}
			Poseidon::JsonObject elem_protection_cost_total;
			for(auto it = protection_cost_total.begin(); it != protection_cost_total.end(); ++it){
				char str[64];
				unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)it->first.get());
				elem_protection_cost_total[SharedNts(str, len)] = it->second;
			}
			elem_castle[sslit("protection_cost_total")] = std::move(elem_protection_cost_total);

			elem_castles[boost::lexical_cast<SharedNts>(castle->get_map_object_uuid())] = std::move(elem_castle);
		}

		root[sslit("castles")] = std::move(elem_castles);
	}

	// 获取背包状态。
	{
		Poseidon::JsonObject elem_items;

		std::vector<ItemBox::ItemInfo> item_info_all;
		item_box->get_all(item_info_all);
		for(auto it = item_info_all.begin(); it != item_info_all.end(); ++it){
			const auto item_id = it->item_id;
			const auto count   = it->count;

			const auto item_data = Data::Item::get(item_id);
			if(!item_data){
				LOG_EMPERY_CENTER_DEBUG("Item data not found: account_uuid = ", item_box->get_account_uuid(), ", item_id = ", item_id);
				continue;
			}
			if((item_data->type.first != Data::Item::CAT_CURRENCY) && (item_data->type.first != Data::Item::CAT_RESOURCE_BOX)){
				continue;
			}
			char str[64];
			unsigned len = (unsigned)std::sprintf(str, "%llu", (unsigned long long)item_id.get());
			elem_items[SharedNts(str, len)] = count;
		}

		root[sslit("items")] = std::move(elem_items);
	}

	return Response();
}

}
