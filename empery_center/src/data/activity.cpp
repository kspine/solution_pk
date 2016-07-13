#include "../precompiled.hpp"
#include "activity.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	template<typename ElementT>
	void read_activiy_type(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.unique_id,     "activity_ID");
		std::string str;
		csv.get(str, "start_time");
		elem.available_since = Poseidon::scan_time(str.c_str());
		csv.get(str, "end_time");
		elem.available_until = Poseidon::scan_time(str.c_str());
	}

	MULTI_INDEX_MAP(ActivityMap, Data::Activity,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(available_since)
	)
	boost::weak_ptr<const ActivityMap> g_activity_map;
	const char ACTIVITY_FILE[] = "activity";

	MULTI_INDEX_MAP(MapActivityMap, Data::MapActivity,
		UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<const MapActivityMap> g_map_activity_map;
	const char MAP_ACTIVITY_FILE[] = "worldmap_activity";

	MULTI_INDEX_MAP(ActivityAwardMap, Data::ActivityAward,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(activity_id)
	)
	boost::weak_ptr<const ActivityAwardMap> g_activity_award_map;
	const char ACTIVITY_AWARD_FILE[] = "rank_reward";

	MULTI_INDEX_MAP(WorldActivityMap, Data::WorldActivity,
		UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<const WorldActivityMap> g_world_activity_map;
	const char WORLD_ACTIVITY_FILE[] = "activity_task";

	MULTI_INDEX_MAP(ActivityContributeMap, Data::ActivityContribute,
	UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<const ActivityContributeMap> g_activity_contribute_map;
	const char ACTIVITY_CONTRIBUTE_FILE[] = "activity_contribution";


	MODULE_RAII_PRIORITY(handles, 1000){

		auto csv = Data::sync_load_data(ACTIVITY_FILE);
		const auto activity_map = boost::make_shared<ActivityMap>();
		while(csv.fetch_row()){
			Data::Activity elem = { };

			read_activiy_type(elem,csv);

			if(!activity_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Activity: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate Activity"));
			}
		}
		g_activity_map = activity_map;
		handles.push(activity_map);
		auto servlet = DataSession::create_servlet(ACTIVITY_FILE, Data::encode_csv_as_json(csv, "activity_ID"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MAP_ACTIVITY_FILE);
		const auto map_activity_map = boost::make_shared<MapActivityMap>();
		while(csv.fetch_row()){
			Data::MapActivity elem = { };
			csv.get(elem.unique_id,                "activity_ID");
			csv.get(elem.activity_type,            "activity_type");
			csv.get(elem.continued_time,           "continued_time");
			Poseidon::JsonObject object;
			csv.get(object, "drop");
			elem.rewards.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				auto condition = boost::lexical_cast<std::uint64_t>(std::string(it->first));
				std::vector<std::pair<std::uint64_t,std::uint64_t>> item_vec;
				const auto &item_obj = it->second.get<Poseidon::JsonObject>();
				item_vec.reserve(item_obj.size());
				for(auto itt = item_obj.begin(); itt != item_obj.end(); ++itt){
					auto item_id = boost::lexical_cast<std::uint64_t>(std::string(itt->first));
					auto num = boost::lexical_cast<std::uint64_t>(itt->second.get<double>());
					item_vec.push_back(std::make_pair(item_id,num));
				}
				if(!elem.rewards.emplace(condition, std::move(item_vec)).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward set: conditon  = ",condition);
					DEBUG_THROW(Exception, sslit("Duplicate reward set"));
				}
			}

			if(!map_activity_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapActivity: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapActivity"));
			}
		}
		g_map_activity_map = map_activity_map;
		handles.push(map_activity_map);
		servlet = DataSession::create_servlet(MAP_ACTIVITY_FILE, Data::encode_csv_as_json(csv, "activity_ID"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(ACTIVITY_AWARD_FILE);
		const auto activity_award_map = boost::make_shared<ActivityAwardMap>();
		while(csv.fetch_row()){
			Data::ActivityAward elem = { };
			csv.get(elem.unique_id,                "id");
			csv.get(elem.activity_id,                "type");
			Poseidon::JsonArray rank_array;
			csv.get(rank_array,                "rank");
			elem.rank_begin = boost::lexical_cast<std::uint64_t>(rank_array.at(0));
			elem.rank_end =  boost::lexical_cast<std::uint64_t>(rank_array.at(1));
			Poseidon::JsonObject object;
			csv.get(object, "reward");
			for(auto it = object.begin(); it != object.end(); ++it){
				auto item_id = boost::lexical_cast<std::uint64_t>(std::string(it->first));
				auto num = boost::lexical_cast<std::uint64_t>(it->second.get<double>());
				elem.rewards.push_back(std::make_pair(item_id,num));
			}

			if(!activity_award_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Activity award: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate Activity award"));
			}
		}
		g_activity_award_map = activity_award_map;
		handles.push(activity_award_map);
		servlet = DataSession::create_servlet(ACTIVITY_AWARD_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(WORLD_ACTIVITY_FILE);
		const auto world_activity_map = boost::make_shared<WorldActivityMap>();
		while(csv.fetch_row()){
			Data::WorldActivity elem = { };
			csv.get(elem.unique_id,                "activity_ID");
			csv.get(elem.pre_unique_id,            "activity_open");
			Poseidon::JsonObject object;
			csv.get(object, "activity_target");
			elem.objective.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				auto id = boost::lexical_cast<std::uint64_t>(it->first.get());
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.objective.emplace(id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate world activity target : id = ", id);
					DEBUG_THROW(Exception, sslit("Duplicate world activity target"));
				}
			}
			object.clear();
			csv.get(object, "drop");
			elem.rewards.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				auto collection_name = std::string(it->first.get());
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.rewards.emplace(std::move(collection_name), count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate world activity reward set: collection_name = ", collection_name);
					DEBUG_THROW(Exception, sslit("Duplicate world activity reward set"));
				}
			}
			if(!world_activity_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Activity : unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate Activity"));
			}
		}
		g_world_activity_map = world_activity_map;
		handles.push(world_activity_map);
		servlet = DataSession::create_servlet(WORLD_ACTIVITY_FILE, Data::encode_csv_as_json(csv, "activity_ID"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(ACTIVITY_CONTRIBUTE_FILE);
		const auto activity_contribute_map = boost::make_shared<ActivityContributeMap>();
		while(csv.fetch_row()){
			Data::ActivityContribute elem = { };
			csv.get(elem.unique_id,                "contribution_id");
			csv.get(elem.factor,                   "output_number");
			csv.get(elem.contribute,               "number");
			if(!activity_contribute_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Activity : unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate Activity"));
			}
		}
		g_activity_contribute_map = activity_contribute_map;
		handles.push(activity_contribute_map);
		servlet = DataSession::create_servlet(ACTIVITY_CONTRIBUTE_FILE, Data::encode_csv_as_json(csv, "contribution_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Activity> Activity::get(std::uint64_t unique_id){
		PROFILE_ME;
		const auto activity_map = g_activity_map.lock();
		if(!activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
			return { };
		}

		const auto it = activity_map->find<0>(unique_id);
		if(it == activity_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapActivityHarvest not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const Activity>(activity_map, &*it);
	}

	boost::shared_ptr<const Activity> Activity::require(std::uint64_t unique_id){
			PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("ActivityMap not found: unique_id = ", unique_id);;
			DEBUG_THROW(Exception, sslit("ActivityMap not found"));
		}
		return ret;
	}

	void Activity::get_all(std::vector<boost::shared_ptr<const Activity>> &ret){
		PROFILE_ME;
		const auto activity_map = g_activity_map.lock();
		if(!activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
			return ;
		}
		ret.reserve(activity_map->size());
		for(auto it = activity_map->begin<0>(); it != activity_map->end<0>(); ++it){
			ret.emplace_back(activity_map, &*it);
		}
	}

	boost::shared_ptr<const MapActivity> MapActivity::get(std::uint64_t unique_id){
		PROFILE_ME;
		const auto map_activity_map = g_map_activity_map.lock();
		if(!map_activity_map){
			LOG_EMPERY_CENTER_WARNING("MapActivityMap has not been loaded.");
			return { };
		}

		const auto it = map_activity_map->find<0>(unique_id);
		if(it == map_activity_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapActivityMap not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const MapActivity>(map_activity_map, &*it);
	}

	boost::shared_ptr<const MapActivity> MapActivity::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapActivityMap not found: unique_id = ", unique_id);;
			DEBUG_THROW(Exception, sslit("MapActivityMap not found"));
		}
		return ret;
	}

	void MapActivity::get_all(std::vector<boost::shared_ptr<const MapActivity>> &ret){
		PROFILE_ME;

		const auto map_activity_map = g_map_activity_map.lock();
		if(!map_activity_map){
			LOG_EMPERY_CENTER_WARNING("MapActivityMap has not been loaded.");
			return;
		}
		ret.reserve(map_activity_map->size());
		for(auto it = map_activity_map->begin<0>(); it != map_activity_map->end<0>(); ++it){
			ret.emplace_back(map_activity_map, &*it);
		}
	}

	bool ActivityAward::get_activity_rank_award(std::uint64_t activity_id,const std::uint64_t rank,std::vector<std::pair<std::uint64_t,std::uint64_t>> &rewards){
		PROFILE_ME;

		const auto activity_award_map = g_activity_award_map.lock();
		if(!activity_award_map){
			LOG_EMPERY_CENTER_WARNING("activity award map has not been loaded.");
			return false;
		}

		const auto range = activity_award_map->equal_range<1>(activity_id);
		for(auto it = range.first; it != range.second; ++it){
			if(it->rank_begin <= rank && it->rank_end >= rank){
				rewards = it->rewards;
				return true;
			}
		}
		return false;
	}

	std::uint64_t ActivityAward::get_max_activity_award_rank(std::uint64_t activity_id){
		PROFILE_ME;

		const auto activity_award_map = g_activity_award_map.lock();
		if(!activity_award_map){
			LOG_EMPERY_CENTER_WARNING("activity award map has not been loaded.");
			return 0;
		}

		std::uint64_t max_rank = 0;
		const auto range = activity_award_map->equal_range<1>(activity_id);
		for(auto it = range.first; it != range.second; ++it){
			if(it->rank_end > max_rank){
				max_rank = it->rank_end;
			}
		}
		return max_rank;
	}

	boost::shared_ptr<const WorldActivity> WorldActivity::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto world_activity_map = g_world_activity_map.lock();
		if(!world_activity_map){
			LOG_EMPERY_CENTER_WARNING("WorldActivityMap has not been loaded.");
			return { };
		}

		const auto it = world_activity_map->find<0>(unique_id);
		if(it == world_activity_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("WorldActivityMap not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const WorldActivity>(world_activity_map, &*it);
	}

	boost::shared_ptr<const WorldActivity> WorldActivity::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("WorldActivityMap not found: unique_id = ", unique_id);;
			DEBUG_THROW(Exception, sslit("WorldActivityMap not found"));
		}
		return ret;
	}

	void WorldActivity::get_all(std::vector<boost::shared_ptr<const WorldActivity>> &ret){
		PROFILE_ME;

		const auto world_activity_map = g_world_activity_map.lock();
		if(!world_activity_map){
			LOG_EMPERY_CENTER_WARNING("WorldActivityMap has not been loaded.");
			return;
		}
		ret.reserve(world_activity_map->size());
		for(auto it = world_activity_map->begin<0>(); it != world_activity_map->end<0>(); ++it){
			ret.emplace_back(world_activity_map, &*it);
		}
	}

	boost::shared_ptr<const ActivityContribute> ActivityContribute::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto activity_contribute_map = g_activity_contribute_map.lock();
		if(!activity_contribute_map){
			LOG_EMPERY_CENTER_WARNING("activity contribute has not been loaded.");
			return {};
		}

		const auto it = activity_contribute_map->find<0>(unique_id);
		if(it == activity_contribute_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("activity contribute not found: unique_id = ", unique_id);
			return {};
		}
		return boost::shared_ptr<const ActivityContribute>(activity_contribute_map, &*it);
	}
}
}
