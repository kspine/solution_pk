#include "../precompiled.hpp"
#include "legion_financial_map.hpp"

#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

#include <tuple>


#include "../player_session.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"

#include "../mysql/legion.hpp"
#include "account_map.hpp"
#include "../account_attribute_ids.hpp"

namespace EmperyCenter{
namespace{
struct LegionFinancialElement{
boost::shared_ptr<MySql::Center_LegionFinancial> legionfinancials;
  LegionFinancialUuid  financial_uuid;
  LegionUuid  legion_uuid;
  AccountUuid account_uuid;
  ItemId item_id;  
std::uint64_t  old_count;
std::uint64_t  new_count;
std::int64_t   delta_count;
std::uint64_t  action_id;
std::uint64_t  action_count;
std::uint64_t  created_time;
explicit LegionFinancialElement(boost::shared_ptr<MySql::Center_LegionFinancial> legionfinancials_)
:legionfinancials(std::move(legionfinancials_))
,financial_uuid(legionfinancials->get_financial_uuid())
,legion_uuid(legionfinancials->get_legion_uuid())
,account_uuid(legionfinancials->get_account_uuid())
,item_id(legionfinancials->get_item_id())
,old_count(legionfinancials->get_old_count())
,new_count(legionfinancials->get_new_count())
,delta_count(legionfinancials->get_delta_count())
,action_id(legionfinancials->get_action_id())
,action_count(legionfinancials->get_action_count())
,created_time(legionfinancials->get_created_time()){}
};

MULTI_INDEX_MAP(LegionFinancialContainer,LegionFinancialElement,
UNIQUE_MEMBER_INDEX(financial_uuid)
MULTI_MEMBER_INDEX(legion_uuid)
)

boost::shared_ptr<LegionFinancialContainer> g_legion_financial_map;


MODULE_RAII_PRIORITY(handles,5000)
{
   const auto conn = Poseidon::MySqlDaemon::create_connection();
   struct TempLegionFinancialElement
   {
      boost::shared_ptr<MySql::Center_LegionFinancial> obj;
   };
   std::map<LegionFinancialUuid,TempLegionFinancialElement> temp_map;

   std::string str_query =  "SELECT * FROM `Center_LegionFinancial` ";
   conn->execute_sql(str_query);

   while(conn->fetch_row())
   {
      auto obj = boost::make_shared<MySql::Center_LegionFinancial>();
      obj->fetch(conn);
      obj->enable_auto_saving();
      const auto financial_uuid = LegionFinancialUuid(obj->get_financial_uuid());
      temp_map[financial_uuid].obj = std::move(obj);
   }

   LOG_EMPERY_CENTER_INFO("Loaded ", temp_map.size(), " LegionFinancial(s).");
   const auto legion_financial_map = boost::make_shared<LegionFinancialContainer>();
   for (const auto &p : temp_map)
   {
     legion_financial_map->insert(LegionFinancialElement(std::move(p.second.obj)));
   }
   g_legion_financial_map = legion_financial_map;
   handles.push(legion_financial_map);
}
}

 void LegionFinancialMap::insert(const boost::shared_ptr<MySql::Center_LegionFinancial> &legionfinancials)
 {
   PROFILE_ME;
   const auto &legion_financial_map = g_legion_financial_map;
   if(!legion_financial_map){
     return;
   }
   if(!legion_financial_map->insert(LegionFinancialElement(legionfinancials)).second){
     return;
   }
 }

 void LegionFinancialMap::make_insert(LegionUuid  legion_uuid,AccountUuid account_uuid,
                             ItemId item_id,std::uint64_t old_count,std::uint64_t new_count,
                             std::int64_t delta_count,std::uint64_t action_id,
                             std::uint64_t action_count,std::uint64_t  created_time)
{
    auto obj = boost::make_shared<MySql::Center_LegionFinancial>((LegionFinancialUuid(Poseidon::Uuid::random())).get(),legion_uuid.get(),account_uuid.get(),item_id.get(), old_count, new_count, delta_count,action_id,action_count, created_time);
    obj->enable_auto_saving();

    Poseidon::MySqlDaemon::enqueue_for_saving(obj, true, true);

    insert(obj);
}

 void  LegionFinancialMap::get_legion_financial(LegionUuid legion_uuid,std::vector<boost::shared_ptr<MySql::Center_LegionFinancial>> &ret)
 {
   PROFILE_ME;
   const auto &legion_financial_map = g_legion_financial_map;
   if(!legion_financial_map){
     return;
   }
   const auto range = legion_financial_map->equal_range<1>(legion_uuid);
   ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
   for (auto it = range.first; it != range.second; ++it)
   {
       ret.emplace_back(it->legionfinancials);
   }
   return;
   }
}