#include "../precompiled.hpp"
#include "legion_store_config.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../id_types.hpp"

namespace EmperyCenter
{
    namespace
    {
        MULTI_INDEX_MAP(LegionStoreContainer, Data::LegionStore,
            UNIQUE_MEMBER_INDEX(goods_id))

        boost::weak_ptr<const LegionStoreContainer> g_LegionStore_container;
	    const char CORPS_STORE_FILE[] = "corps_shop";

        MODULE_RAII_PRIORITY(handles, 1000)
        {
            auto csv = Data::sync_load_data(CORPS_STORE_FILE);
            const auto LegionStore_container = boost::make_shared<LegionStoreContainer>();

            while(csv.fetch_row())
            {
                    Data::LegionStore elem = { };

                    csv.get(elem.goods_id, "goods_id");
                    csv.get(elem.limited_purchase, "limited_purchase");
                    csv.get(elem.trading_id, "trading_id");

                    Poseidon::JsonObject object;

                    // 解锁条件
                    csv.get(object, "unlock_condition");
                    elem.unlock_condition.reserve(object.size());

                    for(auto it = object.begin(); it != object.end(); ++it)
                    {
                        auto collection_name = std::string(it->first.get());
                        const auto count = static_cast<std::uint64_t>(it->second.get<double>());
                        if(!elem.unlock_condition.emplace(std::move(collection_name), count).second)
                        {
                            LOG_EMPERY_CENTER_ERROR("unlock_condition map: collection_name = ", collection_name);
                            DEBUG_THROW(Exception, sslit("Duplicate level_up_cost map"));
                        }
                    }

                    if(!LegionStore_container->insert(std::move(elem)).second)
                    {
                        LOG_EMPERY_CENTER_ERROR("Duplicate LegionStore: goods_id = ", elem.goods_id);
                        DEBUG_THROW(Exception, sslit("Duplicate LegionStore"));
                    }


//                    LOG_EMPERY_CENTER_ERROR("Load  corps_shop.csv : success!!!",elem.goods_id);
            }


		   g_LegionStore_container = LegionStore_container;
		   handles.push(LegionStore_container);
		   auto servlet = DataSession::create_servlet(CORPS_STORE_FILE, Data::encode_csv_as_json(csv, "goods_id"));
		   handles.push(std::move(servlet));

        }
   }

    namespace Data
    {
        boost::shared_ptr<const LegionStore> LegionStore::get(LegionStoreGoodsId goodsid)
        {
		   PROFILE_ME;

		   const auto LegionStore_container = g_LegionStore_container.lock();
		   if(!LegionStore_container)
           {
			 LOG_EMPERY_CENTER_WARNING("LegionStoreContainer has not been loaded.");
			 return { };
		   }

		   const auto it = LegionStore_container->find<0>(goodsid);
		   if(it == LegionStore_container->end<0>())
           {
			 LOG_EMPERY_CENTER_TRACE("Corps not found: corps_level = ", goodsid);
			 return { };
		   }
		   return boost::shared_ptr<const LegionStore>(LegionStore_container, &*it);
        }

	    boost::shared_ptr<const LegionStore> LegionStore::require(LegionStoreGoodsId goodsid)
        {
		   PROFILE_ME;
		   auto ret = get(goodsid);
		   if(!ret)
           {
			 LOG_EMPERY_CENTER_WARNING("Corps not found: corps_level = ", goodsid);
			 DEBUG_THROW(Exception, sslit("Corps not found"));
		   }
		   return ret;
        }
	}
}