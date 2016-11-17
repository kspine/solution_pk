#ifndef EMPERY_CENTER_SINGLETONS_CASTLE_OFFLINE_UPGRADE_BUILDING_BASE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_CASTLE_OFFLINE_UPGRADE_BUILDING_BASE_MAP_HPP_


#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter{

  namespace MySql{

    class Center_CastleOfflineUpgradeBuildingBase;
  }

  class PlayerSession;

  struct CastleOfflineUpgradeBuildingBaseMap{

        static boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> find(AccountUuid account_uuid,MapObjectUuid map_object_uuid);

        static void insert(const boost::shared_ptr<MySql::Center_CastleOfflineUpgradeBuildingBase> &obj);

        static void make_insert(MapObjectUuid map_object_uuid,std::uint64_t building_base_id);

        static void deleted(AccountUuid account_uuid,MapObjectUuid map_object_uuid);

        static void Synchronize_with_player(AccountUuid account_uuid,MapObjectUuid map_object_uuid,const boost::shared_ptr<PlayerSession> &session);

    private:
        CastleOfflineUpgradeBuildingBaseMap() = delete;
  };
}

#endif