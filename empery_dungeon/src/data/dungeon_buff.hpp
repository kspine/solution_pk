#ifndef EMPERY_DUNGEON_DATA_DUNGEON_BUFF_HPP_
#define EMPERY_DUNGEON_DATA_DUNGEON_BUFF_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyDungeon {

namespace Data {
	class DungeonBuff {
	public:
		DungeonBuffTypeId                                 buff_id;
		unsigned                                          target;// 1:单一对象 2：群体 
		std::uint64_t                                     continue_time;
		AttributeId                                       attribute_id;
		std::uint64_t                                     value;
	public:
		static boost::shared_ptr<const DungeonBuff> get(DungeonBuffTypeId buff_id);
		static boost::shared_ptr<const DungeonBuff> require(DungeonBuffTypeId buff_id);
};
}

}

#endif
