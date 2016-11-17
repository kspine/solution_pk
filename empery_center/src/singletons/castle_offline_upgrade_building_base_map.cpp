#include "../precompiled.hpp"

#include "castle_offline_upgrade_building_base_map.hpp"

#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <tuple>
#include "../player_session.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"
#include "../msg/sc_castle.hpp"
#include "../mysql/castle.hpp"
#include "account_map.hpp"
#include "../castle.hpp"

#include "../singletons/world_map.hpp"


namespace EmperyCenter{

   namespace {

       struct CastleOfflineUpgradeBuildingBaseElement{

          boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> castles;
          std::string auto_uuid;
          AccountUuid account_uuid;
          MapObjectUuid map_object_uuid;
          std::string building_base_ids;

          explicit CastleOfflineUpgradeBuildingBaseElement(boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> castles_)
              : castles(std::move(castles_))
              ,auto_uuid(castles->get_auto_uuid())
              , account_uuid(castles->get_account_uuid())
              , map_object_uuid(castles->get_map_object_uuid())
              , building_base_ids(castles->get_building_base_ids())

          {

          }
       };

       MULTI_INDEX_MAP(CastleOfflineUpgradeBuildingBaseContainer,CastleOfflineUpgradeBuildingBaseElement,
          UNIQUE_MEMBER_INDEX(auto_uuid)
          MULTI_MEMBER_INDEX(account_uuid) 
          MULTI_MEMBER_INDEX(map_object_uuid)
       )

      boost::shared_ptr<CastleOfflineUpgradeBuildingBaseContainer> g_castles_map;

      MODULE_RAII_PRIORITY(handles,5000){
       const auto conn = Poseidon::MySqlDaemon::create_connection();

         struct TempCastlesElement {
          boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> obj;
         };

         std::map<std::string,TempCastlesElement> temp_castles_map;

         conn->execute_sql("SELECT * FROM `Center_CastleOfflineUpgradeBuildingBase`");
         while(conn->fetch_row()){
             auto obj = boost::make_shared<MySql::Center_CastleOfflineUpgradeBuildingBase>();
             obj->fetch(conn);
             obj->enable_auto_saving();
             const auto auto_uuid = obj->get_auto_uuid();
             temp_castles_map[auto_uuid].obj = std::move(obj);
         }
         const auto castles_map = boost::make_shared<CastleOfflineUpgradeBuildingBaseContainer>();
         for(auto it = temp_castles_map.begin(); it != temp_castles_map.end(); ++it){
               castles_map->insert(CastleOfflineUpgradeBuildingBaseElement(std::move(it->second.obj)));
         }
         g_castles_map = castles_map;
         handles.push(castles_map);
     }
  }

  void CastleOfflineUpgradeBuildingBaseMap::insert(const boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> &obj)
  {
      PROFILE_ME;
      const auto &castles_map = g_castles_map;
      if(!castles_map){
             LOG_EMPERY_CENTER_WARNING("Center_CastleOfflineUpgradeBuildingBase  not loaded.");
             DEBUG_THROW(Exception, sslit("Center_CastleOfflineUpgradeBuildingBase not loaded"));
      }
      if(!castles_map->insert(CastleOfflineUpgradeBuildingBaseElement(obj)).second){
             DEBUG_THROW(Exception, sslit("Center_CastleOfflineUpgradeBuildingBase already exists"));
      }
   }



  void CastleOfflineUpgradeBuildingBaseMap::make_insert(MapObjectUuid map_object_uuid,std::uint64_t building_base_id)
  {
      PROFILE_ME;

     const auto& map_object = WorldMap::get_map_object(map_object_uuid);
	 if(!map_object)
	 {
	    DEBUG_THROW(Exception, sslit("map object not exists"));
	    return;
	 }

      auto account_uuid = map_object->get_owner_uuid();
      std::string building_base_ids = boost::lexical_cast<std::string>(building_base_id);
      std::string str_auto_uuid = (account_uuid.str() + "," + map_object_uuid.str());
      auto  ptr_castles = find(account_uuid,map_object_uuid);
      if(ptr_castles)
      {
         building_base_ids += ("," + ptr_castles->unlocked_get_building_base_ids());
      }
      auto obj = boost::make_shared<MySql::Center_CastleOfflineUpgradeBuildingBase>(str_auto_uuid,(map_object->get_owner_uuid()).get(),map_object_uuid.get(),building_base_ids); 
      obj->enable_auto_saving();
      Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true); 

      insert(obj);
  }

   boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> CastleOfflineUpgradeBuildingBaseMap::find(AccountUuid account_uuid,MapObjectUuid map_object_uuid)
   {
      PROFILE_ME;
      const auto &castles_map = g_castles_map;
      if(!castles_map){
            LOG_EMPERY_CENTER_WARNING("Center_CastleOfflineUpgradeBuildingBase  not loaded.");
            return { };
      }
      const auto range = castles_map->equal_range<1>(account_uuid);
      for(auto it = range.first; it != range.second; ++it){
         if(AccountUuid(it->castles->unlocked_get_account_uuid()) == account_uuid && 
             MapObjectUuid(it->castles->unlocked_get_map_object_uuid()) == map_object_uuid)
         {
            return it->castles;
         }
      }
      return { };
   }

   void CastleOfflineUpgradeBuildingBaseMap::deleted(AccountUuid account_uuid,MapObjectUuid map_object_uuid)
   {
      PROFILE_ME;
      const auto &castles_map = g_castles_map;
      if(!castles_map){
            LOG_EMPERY_CENTER_WARNING("Center_CastleOfflineUpgradeBuildingBase  not loaded.");
            return;
      }
      const auto range = castles_map->equal_range<1>(account_uuid);
      for(auto it = range.first; it != range.second;){
         if(AccountUuid(it->castles->unlocked_get_account_uuid()) == account_uuid && 
             MapObjectUuid(it->castles->unlocked_get_map_object_uuid()) == map_object_uuid)
         {
            it = castles_map->erase<1>(it);
            break;
         }
         else
         {
           ++it;
         }
      }

      std::string str_auto_uuid =  (account_uuid.str() + "," + map_object_uuid.str());
      std::string strsql = "DELETE FROM Center_CastleOfflineUpgradeBuildingBase WHERE auto_uuid ='";

      strsql += str_auto_uuid;
      strsql += "';";
      Poseidon::MySqlDaemon::enqueue_for_deleting("Center_CastleOfflineUpgradeBuildingBase",strsql);
   }

   void CastleOfflineUpgradeBuildingBaseMap::Synchronize_with_player(AccountUuid account_uuid,MapObjectUuid map_object_uuid,const boost::shared_ptr<PlayerSession> &session)
   {
      const auto &castles_map = g_castles_map;
      if(!castles_map){
            LOG_EMPERY_CENTER_WARNING("Center_CastleOfflineUpgradeBuildingBase  not loaded.");
            return;
      }
      Msg::SC_CastleOfflineUpgradeBuildingBase msg;
      auto  ptr_castles = find(account_uuid,map_object_uuid);
      if(!ptr_castles)
      {
            LOG_EMPERY_CENTER_WARNING("Center_CastleOfflineUpgradeBuildingBase  not find.");
            return;
      }
      msg.map_object_uuid =  map_object_uuid.str();
      msg.building_base_ids =  ptr_castles->unlocked_get_building_base_ids();

      session->send(msg);

      deleted(account_uuid,map_object_uuid);
   }
}