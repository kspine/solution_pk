#ifndef EMPERY_CENTER_SINGLETONS_LEGION_FIIANCIAL_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_FIIANCIAL_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter
{
   namespace MySql
   {
       class Center_LegionFinancial;
   }

   class PlayerSession;

   struct LegionFinancialMap
   {

   static void insert(const boost::shared_ptr<MySql::Center_LegionFinancial> &legionfinancials);

   static void make_insert(LegionUuid  legion_uuid,AccountUuid account_uuid, ItemId item_id,
                             std::uint64_t old_count,std::uint64_t new_count,std::int64_t delta_count,
                             std::uint64_t action_id,std::uint64_t action_count,std::uint64_t  created_time);

   static void  get_legion_financial(LegionUuid legion_uuid,std::vector<boost::shared_ptr<MySql::Center_LegionFinancial>> &ret);

     public:
           enum EIndex
           {
           EIndex_legion_uuid
           };
     private:
           LegionFinancialMap() = delete;
   };
}

#endif //