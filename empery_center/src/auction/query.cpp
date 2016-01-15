#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../singletons/auction_center_map.hpp"
#include "../auction_center.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../resource_ids.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../item_ids.hpp"
#include "../data/global.hpp"
#include "../data/castle.hpp"

namespace EmperyCenter {

AUCTION_SERVLET("query/account", root, session, params){
	const auto platform_id = boost::lexical_cast<PlatformId>(params.at("platform_id"));
	const auto &login_name = params.at("login_name");

	const auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}

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

		const auto auction_center = AuctionCenterMap::require(account->get_account_uuid());
		auction_center->pump_status();

		{
			std::vector<AuctionCenter::TransferInfo> transfers;
			auction_center->get_all_transfers(transfers);

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
				unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->item_id.get());
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
			auction_center->get_all_items(items);

			Poseidon::JsonObject elem_items;
			for(auto it = items.begin(); it != items.end(); ++it){
				char str[64];
				unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->item_id.get());
				elem_items[SharedNts(str, len)] = it->count;
			}
			elem_auction_center[sslit("items")] = std::move(elem_items);
		}

		root[sslit("auction_center")] = std::move(elem_auction_center);
	}

	const auto resource_amount_per_box = Data::Global::as_unsigned(Data::Global::SLOT_AUCTION_TRANSFER_RESOURCE_AMOUNT_PER_BOX);
	const auto item_count_per_box = Data::Global::as_unsigned(Data::Global::SLOT_AUCTION_TRANSFER_ITEM_COUNT_PER_BOX);

	// 获取城堡状态。
	{
		Poseidon::JsonObject elem_castles;

		std::vector<boost::shared_ptr<MapObject>> map_objects;
		WorldMap::get_map_objects_by_owner(map_objects, account->get_account_uuid());
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto castle = boost::dynamic_pointer_cast<Castle>(*it);
			if(!castle){
				continue;
			}
			castle->pump_status();

			Poseidon::JsonObject elem_castle;
			{
				Poseidon::JsonObject elem_resources;

				const auto fill_resource = [&](ResourceId resource_id){
					const auto resource_data = Data::CastleResource::require(resource_id);

					char str[64];
					unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)resource_data->undeployed_item_id.get());
					const double amount = castle->get_resource(resource_id).amount;
					elem_resources[SharedNts(str, len)] = amount / resource_amount_per_box;
				};
				fill_resource(ResourceIds::ID_GRAIN);
				fill_resource(ResourceIds::ID_WOOD);
				fill_resource(ResourceIds::ID_STONE);

				elem_castle[sslit("resources")] = std::move(elem_resources);
			}
			elem_castle[sslit("name")] = castle->get_name();

			elem_castles[boost::lexical_cast<SharedNts>(castle->get_map_object_uuid())] = std::move(elem_castle);
		}

		root[sslit("castles")] = std::move(elem_castles);
	}

	// 获取背包状态。
	{
		Poseidon::JsonObject elem_items;

		const auto item_box = ItemBoxMap::require(account->get_account_uuid());
		item_box->pump_status();

		const auto fill_item = [&](ItemId item_id){
			char str[64];
			unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)item_id.get());
			const double count = item_box->get(item_id).count;
			elem_items[SharedNts(str, len)] = count / item_count_per_box;
		};
		fill_item(ItemIds::ID_GOLD);
		fill_item(ItemIds::ID_SPRING_WATER);

		root[sslit("items")] = std::move(elem_items);
	}

	return Response();
}

}
