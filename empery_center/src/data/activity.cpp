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
}
}
