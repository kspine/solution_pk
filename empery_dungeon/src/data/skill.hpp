#ifndef EMPERY_DUNGEON_DATA_SKILL_HPP_
#define EMPERY_DUNGEON_DATA_SKILL_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyDungeon {

namespace Data {
	class Skill {
	public:
		DungeonMonsterSkillId                             skill_id;
		std::uint32_t                                     cast_type;     //1:主动 2：被动 
		std::pair<std::uint32_t,std::uint32_t>            cast_interval; //间隔
		std::uint32_t                                     cast_object;   //释放对象
		std::uint32_t                                     cast_range;    //施法范围
		double                                            sing_time;     //吟唱
		double                                            cast_time;     //施法
	public:
		static boost::shared_ptr<const Skill> get(DungeonMonsterSkillId skill_id);
		static boost::shared_ptr<const Skill> require(DungeonMonsterSkillId skill_id);
		static void get_all(std::vector<boost::shared_ptr<const Skill>> &ret);
};
}

}

#endif
