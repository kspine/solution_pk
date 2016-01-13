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
		Poseidon::JsonObject temp_object;

		temp_object[sslit("account_uuid")]    = account->get_account_uuid().str();
		temp_object[sslit("promotion_level")] = account->get_promotion_level();
		temp_object[sslit("nick")]            = account->get_nick();

		root[sslit("account")] = std::move(temp_object);
	}

	// 获取交易中心状态。
	{
		Poseidon::JsonObject temp_object;

		const auto auction_center = AuctionCenterMap::require(account->get_account_uuid());
		auction_center->pump_status();

		{
			std::vector<AuctionCenter::ItemInfo> items;
			auction_center->get_all_items(items);

			Poseidon::JsonObject temp_object;
			for(auto it = items.begin(); it != items.end(); ++it){
				char str[64];
				unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->item_id.get());
				temp_object[SharedNts(str, len)] = it->count;
			}
			temp_object[sslit("items")] = std::move(temp_object);
		}

		{
			std::vector<AuctionCenter::TransferInfo> transfers;
			auction_center->get_all_transfers(transfers);

			boost::container::flat_map<MapObjectUuid, Poseidon::JsonObject> temp_map;
			temp_map.reserve(transfers.size());
			for(auto it = transfers.begin(); it != transfers.end(); ++it){
				auto &temp_object = temp_map[it->map_object_uuid];

				Poseidon::JsonObject item_object;
				item_object[sslit("item_count_locked")]      = it->item_count_locked;
				item_object[sslit("item_count_fee")]         = it->item_count_fee;
				item_object[sslit("resource_id")]            = it->resource_id.get();
				item_object[sslit("resource_amount_locked")] = it->resource_amount_locked;
				item_object[sslit("resource_amount_fee")]    = it->resource_amount_fee;
				item_object[sslit("created_time")]           = it->created_time;
				item_object[sslit("due_time")]               = it->due_time;

				char str[64];
				unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->item_id.get());
				temp_object[SharedNts(str, len)] = std::move(item_object);
			}

			Poseidon::JsonObject castle_object;
			for(auto it = temp_map.begin(); it != temp_map.end(); ++it){
				castle_object[boost::lexical_cast<SharedNts>(it->first)] = std::move(it->second);
			}
			temp_object[sslit("castles")] = std::move(castle_object);
		}

		root[sslit("auction_center")] = std::move(temp_object);
	}

	// 获取城堡状态。
	{
		Poseidon::JsonObject castle_object;

		std::vector<boost::shared_ptr<MapObject>> map_objects;
		WorldMap::get_map_objects_by_owner(map_objects, account->get_account_uuid());
		for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
			const auto castle = boost::dynamic_pointer_cast<Castle>(*it);
			if(!castle){
				continue;
			}
			castle->pump_status();

			Poseidon::JsonObject temp_object;

			{
				Poseidon::JsonObject resource_object;

				const auto fill_resource = [&](ResourceId resource_id){
					char str[64];
					unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)resource_id.get());
					resource_object[SharedNts(str, len)] = castle->get_resource(resource_id).amount;
				};

				fill_resource(ResourceIds::ID_GRAIN);
				fill_resource(ResourceIds::ID_WOOD);
				fill_resource(ResourceIds::ID_STONE);

				temp_object[sslit("resources")] = std::move(resource_object);
			}

			temp_object[sslit("name")] = castle->get_name();

			castle_object[boost::lexical_cast<SharedNts>(castle->get_map_object_uuid())] = std::move(temp_object);
		}

		root[sslit("castles")] = std::move(castle_object);
	}

	// 获取背包状态。
	{
		Poseidon::JsonObject temp_object;

		const auto item_box = ItemBoxMap::require(account->get_account_uuid());
		item_box->pump_status();

		const auto fill_item = [&](ItemId item_id){
			char str[64];
			unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)item_id.get());
			temp_object[SharedNts(str, len)] = item_box->get(item_id).count;
		};

		fill_item(ItemIds::ID_GOLD);
		fill_item(ItemIds::ID_SPRING_WATER);

		root[sslit("items")] = std::move(temp_object);
	}

	return Response();
}

}
