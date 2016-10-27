#ifndef EMPERY_DUNGEON_DATA_TRIGGER_HPP_
#define EMPERY_DUNGEON_DATA_TRIGGER_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyDungeon {

namespace Data {
	class Trigger {
	public:
		std::string                                       dungeon_trigger;
		std::uint64_t                                     trigger_id;
		std::string                                       trigger_name;
		std::pair<std::string,std::uint64_t>              dungeon_trigger_pair;
		unsigned                                          type;
		std::uint64_t                                     delay;
		std::string                                       condition;
		std::string                                       effect;
		std::string                                       effect_params;
		bool                                              activated = false;
		int                                               times;
		bool                                              open;
		int                                               status;
	public:
		static boost::shared_ptr<const Trigger> get(std::string dungeon_trigger,std::uint64_t trigger_id);
		static boost::shared_ptr<const Trigger> require(std::string dungeon_trigger,std::uint64_t  trigger_id);
		static void get_all(std::vector<boost::shared_ptr<const Trigger>> &ret,const std::string dungeon_trigger);
};
}

}

#endif
