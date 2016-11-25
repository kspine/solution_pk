#include "../precompiled.hpp"
#include "activity.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../activity_session.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../mysql/activity_config.hpp"
#include "../singletons/activity_map.hpp"

namespace EmperyCenter {

namespace {
    SharedNts FormatSharedNts(std::uint64_t key){
		char str[256];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)key);
		return SharedNts(str, len);
	}
	MULTI_INDEX_MAP(ActivityDataMap, Data::Activity,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(available_since)
	)
	boost::weak_ptr<ActivityDataMap> g_activity_map;
	const char ACTIVITY_FILE[] = "activity";

	MULTI_INDEX_MAP(MapActivityMap, Data::MapActivity,
		UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<MapActivityMap> g_map_activity_map;
	const char MAP_ACTIVITY_FILE[] = "worldmap_activity";

	MULTI_INDEX_MAP(ActivityAwardMap, Data::ActivityAward,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(activity_id)
	)
	boost::weak_ptr<ActivityAwardMap> g_activity_award_map;
	const char ACTIVITY_AWARD_FILE[] = "rank_reward";

	MULTI_INDEX_MAP(WorldActivityMap, Data::WorldActivity,
		UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<WorldActivityMap> g_world_activity_map;
	const char WORLD_ACTIVITY_FILE[] = "activity_task";

	MULTI_INDEX_MAP(ActivityContributeMap, Data::ActivityContribute,
	UNIQUE_MEMBER_INDEX(unique_id)
	)
	boost::weak_ptr<const ActivityContributeMap> g_activity_contribute_map;
	const char ACTIVITY_CONTRIBUTE_FILE[] = "activity_contribution";


	MODULE_RAII_PRIORITY(handles, 1000){
		const auto activity_map = boost::make_shared<ActivityDataMap>();
		g_activity_map = activity_map;
		handles.push(activity_map);

		const auto map_activity_map = boost::make_shared<MapActivityMap>();
		g_map_activity_map = map_activity_map;
		handles.push(map_activity_map);

		const auto world_activity_map = boost::make_shared<WorldActivityMap>();
		g_world_activity_map = world_activity_map;
		handles.push(world_activity_map);

		const auto activity_award_map = boost::make_shared<ActivityAwardMap>();
		g_activity_award_map = activity_award_map;
		handles.push(activity_award_map);

		auto csv = Data::sync_load_data(ACTIVITY_CONTRIBUTE_FILE);
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
		auto servlet = DataSession::create_servlet(ACTIVITY_CONTRIBUTE_FILE, Data::encode_csv_as_json(csv, "contribution_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {

	void Activity::reload(){
		PROFILE_ME;
		auto activity_map = g_activity_map.lock();
		if(!activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityDataMap has not been loaded.");
			return;
		}
		activity_map->clear();
		/*
		[[3500001,"2016-9-18 7:40:00","2016-9-18 23:30"],
		[3500002,"2016-9-18 7:40:00","2016-9-18 23:30"]]
		*/
		Poseidon::JsonArray activitys;
		ActivityMap::force_load_activitys(activitys);
		std::string test_activity = activitys.dump();
		LOG_EMPERY_CENTER_FATAL("test activity data:",test_activity);
		std::istringstream iss(test_activity);
		auto response_array = Poseidon::JsonParser::parse_array(iss);
		for(auto it = response_array.begin(); it != response_array.end();++it){
			auto activity_array = (*it).get<Poseidon::JsonArray>();
			if(activity_array.size() != 3){
				LOG_EMPERY_CENTER_FATAL("unvalid activity data");
				continue;
			}
			Data::Activity elem = { };
			elem.unique_id       = static_cast<std::uint64_t>(activity_array.at(0).get<double>());
			elem.available_since = static_cast<std::uint64_t>(activity_array.at(1).get<double>());
			elem.available_until = static_cast<std::uint64_t>(activity_array.at(2).get<double>());
			if(!activity_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Activity: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate Activity"));
			}
		}
		ActivitySession::create_servlet(ACTIVITY_FILE,response_array);
	}
	boost::shared_ptr<const Activity> Activity::get(std::uint64_t unique_id){
		PROFILE_ME;
		const auto activity_map = g_activity_map.lock();
		if(!activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityDataMap has not been loaded.");
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
			LOG_EMPERY_CENTER_WARNING("ActivityDataMap not found: unique_id = ", unique_id);;
			DEBUG_THROW(Exception, sslit("ActivityDataMap not found"));
		}
		return ret;
	}

	void Activity::get_all(std::vector<boost::shared_ptr<const Activity>> &ret){
		PROFILE_ME;
		const auto activity_map = g_activity_map.lock();
		if(!activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityDataMap has not been loaded.");
			return ;
		}
		ret.reserve(activity_map->size());
		for(auto it = activity_map->begin<0>(); it != activity_map->end<0>(); ++it){
			ret.emplace_back(activity_map, &*it);
		}
	}

	void MapActivity::reload(){
		PROFILE_ME;

		auto map_activity_map = g_map_activity_map.lock();
		if(!map_activity_map){
			LOG_EMPERY_CENTER_WARNING("map activity data has not been loaded.");
			return;
		}
		map_activity_map->clear();
		/*
		[[3501001,1,10,{}],
		[3502001,2,10,{}],
		[3503001,3,10,{}],
		[3504001,4,30,{}],
        [3505001,5,0,{"5000":{"2100034":5},"10000":{"2100034":10}}],
		[3506001,6,0,{"5000":{"2100034":5},"10000":{"2100034":10}}]];
		*/
		Poseidon::JsonArray activitys;
		ActivityMap::force_load_activitys_map(activitys);
		std::string test_data = activitys.dump();
		LOG_EMPERY_CENTER_FATAL("test map activity data:",test_data);
		std::istringstream map_activity_iss(test_data);
		auto response_array = Poseidon::JsonParser::parse_array(map_activity_iss);
		for(auto it = response_array.begin(); it != response_array.end();++it){
			auto activity_array = (*it).get<Poseidon::JsonArray>();
			if(activity_array.size() != 4){
				LOG_EMPERY_CENTER_FATAL("unvalid activity data");
				continue;
			}
			Data::MapActivity elem = { };
			elem.unique_id       = static_cast<std::uint64_t>(activity_array.at(0).get<double>());
			elem.activity_type   = static_cast<unsigned>(activity_array.at(1).get<double>());
			elem.continued_time  = static_cast<std::uint64_t>(activity_array.at(2).get<double>());
			auto target_str = activity_array.at(3).get<std::string>();
			std::istringstream target_iss(target_str);
			auto object = Poseidon::JsonParser::parse_object(target_iss);
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
				LOG_EMPERY_CENTER_ERROR("Duplicate Activity: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate Activity"));
			}
		}
		ActivitySession::create_servlet(MAP_ACTIVITY_FILE,response_array);
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

	void ActivityAward::reload(){
		PROFILE_ME;
		auto activity_award_map = g_activity_award_map.lock();
		if(!activity_award_map){
			LOG_EMPERY_CENTER_WARNING("activity award has not been loaded.");
			return;
		}
		activity_award_map->clear();
		/*
		[[3600001,3505001,[1,1],{"2100034":5}],
		 [3600002,3505001,[2,2],{"2100034":4}],
		 [3600003,3505001,[3,3],{"2100034":3}],
		 [3600004,3505001,[4,50],{"2100034":2}],
		 [3600005,3500002,[1,1],{"2204004":100}],
		 [3600006,3500002,[2,2],{"2204004":50}],
		 [3600007,3500002,[3,3],{"2204004":20}],
		 [3600008,3500002,[4,10],{"2204004":10}],
		 [3600009,3500002,[11,50],{"2204004":5}],
		 [36000010,3500002,[50,100],{"2204004":2}]
		 ]
		*/
		Poseidon::JsonArray rewards;
		ActivityMap::force_load_activitys_rank_award(rewards);
		std::string test_rewards = rewards.dump();
		LOG_EMPERY_CENTER_FATAL("test activity reward data:",test_rewards);
		std::istringstream iss(test_rewards);
		auto response_array = Poseidon::JsonParser::parse_array(iss);
		for(auto it = response_array.begin(); it != response_array.end();++it){
			auto reward_array = (*it).get<Poseidon::JsonArray>();
			if(reward_array.size() != 4){
				LOG_EMPERY_CENTER_FATAL("unvalid activity reward data:",reward_array.dump());
				continue;
			}
			Data::ActivityAward elem = { };
			elem.unique_id       = static_cast<std::uint64_t>(reward_array.at(0).get<double>());
			elem.activity_id     = static_cast<std::uint64_t>(reward_array.at(1).get<double>());
			auto rank_array = reward_array.at(2).get<Poseidon::JsonArray>();
			if(rank_array.size() != 2){
				LOG_EMPERY_CENTER_FATAL("unvalid activity reward rank data:",rank_array.dump());
				continue;
			}
			elem.rank_begin = boost::lexical_cast<std::uint64_t>(rank_array.at(0));
			elem.rank_end =  boost::lexical_cast<std::uint64_t>(rank_array.at(1));
			auto reward_str = reward_array.at(3).get<std::string>();
			std::istringstream reward_iss(reward_str);
			auto object = Poseidon::JsonParser::parse_object(reward_iss);

			elem.rewards.reserve(object.size());
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
		ActivitySession::create_servlet(ACTIVITY_AWARD_FILE,response_array);
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

void ActivityAward::get_activity_award_all(std::uint64_t activity_id,std::vector<boost::shared_ptr<const ActivityAward>> &ret){
		PROFILE_ME;

		const auto activity_award_map = g_activity_award_map.lock();
		if(!activity_award_map){
			LOG_EMPERY_CENTER_WARNING("activity award map has not been loaded.");
			return;
		}

		const auto range = activity_award_map->equal_range<1>(activity_id);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(activity_award_map, &*it);
		}
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

	void WorldActivity::reload(){
		PROFILE_ME;
		auto world_activity_map = g_world_activity_map.lock();
		if(!world_activity_map){
			LOG_EMPERY_CENTER_WARNING("world activity map has not been loaded.");
			return;
		}
		world_activity_map->clear();
		/*
		[[3510001,0,3500002,{"0":500000},1,{"guaibag10":1,"liangbag10":1,"mubag10":1,"shibag10":1}],
		[3511001,3510001,3500002,{"0":1000000},2,{"guaibag20":1,"liangbag20":1,"mubag20":1,"shibag20":1}],
		[3512001,3511001,3500002,{"2605102":1},3,{"guaibag30":1,"liangbag30":1,"mubag30":1,"shibag30":1}]]
		*/
		Poseidon::JsonArray activitys;
		ActivityMap::force_load_activitys_world(activitys);
		std::string test_data = activitys.dump();
		LOG_EMPERY_CENTER_FATAL("test world activity:",test_data);
		std::istringstream world_activity_iss(test_data);
		auto response_array = Poseidon::JsonParser::parse_array(world_activity_iss);
		for(auto it = response_array.begin(); it != response_array.end();++it){
			auto activity_array = (*it).get<Poseidon::JsonArray>();
			if(activity_array.size() != 6){
				LOG_EMPERY_CENTER_FATAL("unvalid activity data");
				continue;
			}
			Data::WorldActivity elem = { };
			elem.unique_id       = static_cast<std::uint64_t>(activity_array.at(0).get<double>());
			elem.pre_unique_id   = static_cast<unsigned>(activity_array.at(1).get<double>());

			auto target_str = activity_array.at(3).get<std::string>();
			std::istringstream target_iss(target_str);
			auto object = Poseidon::JsonParser::parse_object(target_iss);

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
			auto drop_str = activity_array.at(5).get<std::string>();
			std::istringstream drop_iss(drop_str);
			object = Poseidon::JsonParser::parse_object(drop_iss);
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
			ActivitySession::create_servlet(WORLD_ACTIVITY_FILE,response_array);
		}
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
