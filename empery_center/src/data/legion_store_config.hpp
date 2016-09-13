#ifndef EMPERY_CENTER_DATA_LEGION_STORE_IN_HPP_
#define EMPERY_CENTER_DATA_LEGION_STORE_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
    namespace Data
    {
        class LegionStore
        {
            public:
               static boost::shared_ptr<const LegionStore> get(LegionStoreGoodsId goodsid);
               static boost::shared_ptr<const LegionStore> require(LegionStoreGoodsId goodsid);
            public:
               LegionStoreGoodsId goods_id;

               std::uint64_t  trading_id;
               std::uint64_t  limited_purchase;
               boost::container::flat_map<std::string, std::uint64_t> unlock_condition;
        };
    }
}
#endif //
