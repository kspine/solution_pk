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

		std::vector<std::pair<std::int32_t, std::int32_t>> start_points;
		std::pair<std::uint64_t, std::uint64_t> soldier_count_limits;
		boost::container::flat_multimap<ApplicabilityKeyType, std::uint64_t> battalions_required;
		boost::container::flat_multimap<ApplicabilityKeyType, std::uint64_t> battalions_forbidden;

		boost::container::flat_set<DungeonTaskId> tasks;
		boost::container::flat_map<ItemId, std::uint64_t> rewards;
		boost::container::flat_map<ResourceId, std::uint64_t> rewards_resources;
		double resuscitation_ratio;
		boost::container::flat_set<DungeonTaskId> need_tasks;
	};

	class DungeonTask {
	public:
		static boost::shared_ptr<const DungeonTask> get(DungeonTaskId dungeon_task_id);
		static boost::shared_ptr<const DungeonTask> require(DungeonTaskId dungeon_task_id);

	public:
		DungeonTaskId dungeon_task_id;
		boost::container::flat_map<ItemId, std::uint64_t> rewards;
		boost::container::flat_map<ResourceId, std::uint64_t> rewards_resources;
	};

	class DungeonTrap {
	public:
		static boost::shared_ptr<const DungeonTrap> get(DungeonTrapTypeId dungeon_trap_type_id);
		static boost::shared_ptr<const DungeonTrap> require(DungeonTrapTypeId dungeon_trap_type_id);
	public:
		DungeonTrapTypeId trap_type_id;
		unsigned          attack_type;
		boost::uint64_t   attack_range;
		boost::uint64_t   attack;
	};
}

}

#endif
