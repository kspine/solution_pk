#ifndef EMPERY_CENTER_DATA_DUNGEON_HPP_
#define EMPERY_CENTER_DATA_DUNGEON_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include "applicability_key_type.hpp"

namespace EmperyCenter {

namespace Data {
	class Dungeon {
	public:
		static boost::shared_ptr<const Dungeon> get(DungeonTypeId dungeon_type_id);
		static boost::shared_ptr<const Dungeon> require(DungeonTypeId dungeon_type_id);

	public:
		DungeonTypeId dungeon_type_id;
		bool reentrant;
		DungeonTypeId prerequisite_dungeon_type_id;
		boost::container::flat_map<ItemId, std::uint64_t> entry_cost;
		// std::string map_name;

		unsigned battalion_count_limit;
		std::pair<std::uint64_t, std::uint64_t> soldier_count_limits;
		boost::container::flat_multimap<ApplicabilityKeyType, std::uint64_t> battalions_required;
		boost::container::flat_multimap<ApplicabilityKeyType, std::uint64_t> battalions_forbidden;

		boost::container::flat_set<DungeonTaskId> tasks;
		boost::container::flat_map<std::string, std::uint64_t> rewards;
		double resuscitation_ratio;
	};

	class DungeonTask {
	public:
		static boost::shared_ptr<const DungeonTask> get(DungeonTaskId dungeon_task_id);
		static boost::shared_ptr<const DungeonTask> require(DungeonTaskId dungeon_task_id);

	public:
		DungeonTaskId dungeon_task_id;
		boost::container::flat_map<ItemId, std::uint64_t> rewards;
	};
}

}

#endif
