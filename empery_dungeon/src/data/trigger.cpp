#include "../precompiled.hpp"
#include "trigger.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyDungeon {

namespace {
	template<typename DungeonTriggerContainerT>
	void read_dungeon_trigger_data(DungeonTriggerContainerT &container,std::string dungeon_trigger){
		auto csv = Data::sync_load_data(dungeon_trigger.c_str());
		while(csv.fetch_row()){
			Data::Trigger elem = { };
			elem.dungeon_trigger = dungeon_trigger;
			csv.get(elem.trigger_id,           "id");
			csv.get(elem.trigger_name,         "name");
			elem.dungeon_trigger_pair = std::make_pair(dungeon_trigger,elem.trigger_id);
			csv.get(elem.type,             "trigger_type");
			csv.get(elem.delay,            "delay");
			csv.get(elem.condition,         "condition");
			csv.get(elem.effect,           "effect_type");
			csv.get(elem.effect_params,    "effect_params");
			csv.get(elem.times,            "times");
			csv.get(elem.open,             "open");
			csv.get(elem.status,           "status");
			std::uint64_t trigger_id = elem.trigger_id;
			if(!container->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate dungeon trigger  dungeon_trigger = ", dungeon_trigger," trigger_id = ",trigger_id);
				DEBUG_THROW(Exception, sslit("Duplicate dungeon trigger"));
			}
		}
	}
	MULTI_INDEX_MAP(DungeonTriggerContainer, Data::Trigger,
		UNIQUE_MEMBER_INDEX(dungeon_trigger_pair)
		MULTI_MEMBER_INDEX(dungeon_trigger)
	)
	boost::weak_ptr<const DungeonTriggerContainer> g_dungeon_trigger_container;

	const auto MAX_DUNGE_TRIGGER_COUNT = 28;
	const std::string  DUNGEON_TRIGGER_FILES[MAX_DUNGE_TRIGGER_COUNT] = {
		"dungeon_trigger_0_1","dungeon_trigger_0_2","dungeon_trigger_0_3","dungeon_trigger_0_4","dungeon_trigger_0_5","dungeon_trigger_0_6","dungeon_trigger_0_7",
		"dungeon_trigger_1_1","dungeon_trigger_1_2","dungeon_trigger_1_3","dungeon_trigger_1_4","dungeon_trigger_1_5","dungeon_trigger_1_6",
		"dungeon_trigger_2_1","dungeon_trigger_2_2","dungeon_trigger_2_3","dungeon_trigger_2_4","dungeon_trigger_2_5","dungeon_trigger_2_6",
		"dungeon_trigger_3_1","dungeon_trigger_3_2","dungeon_trigger_3_3","dungeon_trigger_3_4","dungeon_trigger_3_5","dungeon_trigger_3_6",
		"dungeon_trigger_4_1","dungeon_trigger_4_2","dungeon_trigger_4_3"};

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto dungeon_trigger_container = boost::make_shared<DungeonTriggerContainer>();
		for(auto i = 0; i < MAX_DUNGE_TRIGGER_COUNT;i++){
			read_dungeon_trigger_data(dungeon_trigger_container,DUNGEON_TRIGGER_FILES[i]);
		}
		g_dungeon_trigger_container = dungeon_trigger_container;
		handles.push(dungeon_trigger_container);
	}
}

namespace Data {
	boost::shared_ptr<const Trigger> Trigger::get(std::string dungeon_trigger,std::uint64_t trigger_id){
		PROFILE_ME;

		const auto dungeon_trigger_container = g_dungeon_trigger_container.lock();
		if(!dungeon_trigger_container){
			LOG_EMPERY_DUNGEON_WARNING("dungeon_trigger_container has not been loaded.");
			return { };
		}

		const auto it = dungeon_trigger_container->find<0>(std::make_pair(dungeon_trigger,trigger_id));
		if(it == dungeon_trigger_container->end<0>()){
			LOG_EMPERY_DUNGEON_TRACE("dugeon trigger not found: dungeon_trigger = ", dungeon_trigger," trigger_id = ",trigger_id);
			return { };
		}
		return boost::shared_ptr<const Trigger>(dungeon_trigger_container, &*it);
	}

	boost::shared_ptr<const Trigger> Trigger::require(std::string dungeon_trigger,std::uint64_t trigger_id){
		PROFILE_ME;

		auto ret = get(dungeon_trigger,trigger_id);
		if(!ret){
			LOG_EMPERY_DUNGEON_WARNING("dungeon trigger not found: dungeon_trigger = ", dungeon_trigger," trigger_id = ", trigger_id);
			DEBUG_THROW(Exception, sslit("dungeon trigger not found"));
		}
		return ret;
	}

	void Trigger::get_all(std::vector<boost::shared_ptr<const Trigger>> &ret,const std::string dungeon_trigger){
		PROFILE_ME;

		const auto dungeon_trigger_container = g_dungeon_trigger_container.lock();
		if(!dungeon_trigger_container){
			LOG_EMPERY_DUNGEON_WARNING("dungeon_trigger_container has not been loaded.");
			return ;
		}
		const auto range = dungeon_trigger_container->equal_range<1>(dungeon_trigger);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(dungeon_trigger_container,&*it);
		}
	}
}

}
