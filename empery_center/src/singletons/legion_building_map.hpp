#ifndef EMPERY_CENTER_SINGLETONS_LEGION_BUILDING_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_BUILDING_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"
#include "../legion_member.hpp"

namespace EmperyCenter {


namespace MySql {
	class Center_LegionBuilding;
}

class PlayerSession;


struct LegionBuildingMap {

	static boost::shared_ptr<MySql::Center_LegionBuilding> find(LegionUuid legion_uuid);

	static std::uint64_t get_apply_count(LegionUuid account_uuid);

    static void  find_by_type(std::vector<boost::shared_ptr<MySql::Center_LegionBuilding>> &buildings,LegionUuid legion_uuid,std::uint64_t ntype);

	static void insert(const boost::shared_ptr<MySql::Center_LegionBuilding> &building);
	static void deleteInfo(LegionUuid legion_uuid,AccountUuid account_uuid,bool bAll = false);
	static void deleteInfo_by_legion_uuid(LegionUuid legion_uuid);

	static void synchronize_with_player(LegionUuid legion_uuid,const boost::shared_ptr<PlayerSession> &session);



private:
	LegionBuildingMap() = delete;
};

}

#endif
