#include "../precompiled.hpp"
#include "activity.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../singletons/simple_http_client_daemon.hpp"
#include "../activity_session.hpp"
#include "../mmain.hpp"

namespace EmperyCenter {

namespace {
	/*
	template<typename ElementT>
	void read_activiy_type(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.unique_id,     "activity_ID");
		std::string str;
		csv.get(str, "start_time");
		elem.available_since = Poseidon::scan_time(str.c_str());
		csv.get(str, "end_time");
		elem.available_until = Poseidon::scan_time(str.c_str());
	}
	*/
    SharedNts FormatSharedNts(std::uint64_t key){
		char str[256];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)key);
		return SharedNts(str, len);
	}
	MULTI_INDEX_MAP(ActivityMap, Data::Activity,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(available_since)
	)
	boost::weak_ptr<ActivityMap> g_activity_map;
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
	boost::weak_ptr<const ActivityAwardMap> g_activity_award_map;
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
		const auto activity_map = boost::make_shared<ActivityMap>();
		g_activity_map = activity_map;
		handles.push(activity_map);

		const auto map_activity_map = boost::make_shared<MapActivityMap>();
		g_map_activity_map = map_activity_map;
		handles.push(map_activity_map);

		const auto world_activity_map = boost::make_shared<WorldActivityMap>();
		g_world_activity_map = world_activity_map;
		handles.push(world_activity_map);

		auto csv = Data::sync_load_data(ACTIVITY_AWARD_FILE);
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
		auto servlet = DataSession::create_servlet(ACTIVITY_AWARD_FILE, Data::encode_csv_as_json(csv, "id"));
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

	void Activity::reload(){
		PROFILE_ME;
		auto activity_map = g_activity_map.lock();
		if(!activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
			return;
		}
		activity_map->clear();
		/*
		Poseidon::OptionalMap get_params;
		get_params.set(sslit("server"), "102");
		get_params.set(sslit("channel"), "1000");
		auto entity = SimpleHttpClientDaemon::sync_request(g_server_activity_host, g_server_activity_port, g_server_activity_use_ssl,
		Poseidon::Http::V_GET, g_server_activity_path + "/get_activity_info", std::move(get_params), g_server_activity_auth);
		std::istringstream iss(entity.dump());
		*/
		/*
		[[3500001,"2016-9-18 7:40:00","2016-9-18 23:30"],
		[3500002,"2016-9-18 7:40:00","2016-9-18 23:30"]]
		*/
		Poseidon::JsonArray activitys;
		//活动1
		Poseidon::JsonArray activity1;
		activity1.emplace_back(3500001);
		activity1.emplace_back("2016-9-20 5:40:00");
		activity1.emplace_back("2016-9-20 23:30:00");
		activitys.emplace_back(activity1);
		//活动2
		Poseidon::JsonArray activity2;
		activity2.emplace_back(3500002);
		activity2.emplace_back("2016-9-20 5:40:00");
		activity2.emplace_back("2016-9-20 23:30:00");
		activitys.emplace_back(activity2);
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
			elem.available_since = Poseidon::scan_time(activity_array.at(1).get<std::string>().c_str());
			elem.available_until = Poseidon::scan_time(activity_array.at(2).get<std::string>().c_str());
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

	void MapActivity::reload(){
		PROFILE_ME;

		auto map_activity_map = g_map_activity_map.lock();
		if(!map_activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
			return;
		}
		map_activity_map->clear();
		/*
		Poseidon::OptionalMap get_params;
		get_params.set(sslit("server"), "102");
		get_params.set(sslit("channel"), "1000");
		auto map_activity_entity = SimpleHttpClientDaemon::sync_request(g_server_activity_host, g_server_activity_port, g_server_activity_use_ssl,
		Poseidon::Http::V_GET, g_server_activity_path + "/get_map_activity_info", std::move(get_params), g_server_activity_auth);
		std::istringstream map_activity_iss(map_activity_entity.dump());
		*/
		/*
		[[3501001,1,10,{}],
		[3502001,2,10,{}],
		[3503001,3,10,{}],
		[3504001,4,30,{}],
        [3505001,5,0,{"5000":{"2100034":5},"10000":{"2100034":10}}],
		[3506001,6,0,{"5000":{"2100034":5},"10000":{"2100034":10}}]];
		*/
		Poseidon::JsonArray activitys;
		// 活动1
		Poseidon::JsonArray activity1;
		activity1.emplace_back(3501001);
		activity1.emplace_back(1);
		activity1.emplace_back(10);
		Poseidon::JsonObject objective1;
		activity1.emplace_back(objective1);
		activitys.emplace_back(activity1);
		//活动2
		Poseidon::JsonArray activity2;
		activity2.emplace_back(3502001);
		activity2.emplace_back(2);
		activity2.emplace_back(10);
		Poseidon::JsonObject objective2;
		activity2.emplace_back(objective2);
		activitys.emplace_back(activity2);
		//活动3
		Poseidon::JsonArray activity3;
		activity3.emplace_back(3503001);
		activity3.emplace_back(3);
		activity3.emplace_back(10);
		Poseidon::JsonObject objective3;
		activity3.emplace_back(objective3);
		activitys.emplace_back(activity3);
		//活动4
		Poseidon::JsonArray activity4;
		activity4.emplace_back(3504001);
		activity4.emplace_back(4);
		activity4.emplace_back(10);
		Poseidon::JsonObject objective4;
		activity4.emplace_back(objective4);
		activitys.emplace_back(activity4);
		//活动5
		Poseidon::JsonArray activity5;
		activity5.emplace_back(3505001);
		activity5.emplace_back(5);
		activity5.emplace_back(0);
		Poseidon::JsonObject objective5;
		Poseidon::JsonObject objective51;
		objective51[FormatSharedNts(2100034)] = 5;
		objective5[FormatSharedNts(5000)] = objective51;
		Poseidon::JsonObject objective52;
		objective52[FormatSharedNts(2100034)] = 10;
		objective5[FormatSharedNts(10000)] = objective52;
		activity5.emplace_back(objective5);
		activitys.emplace_back(activity5);
		//活动6
		Poseidon::JsonArray activity6;
		activity6.emplace_back(3506001);
		activity5.emplace_back(6);
		activity5.emplace_back(0);
		Poseidon::JsonObject objective6;
		Poseidon::JsonObject objective61;
		objective61[FormatSharedNts(2100034)] = 5;
		objective6[FormatSharedNts(5000)] = objective61;
		Poseidon::JsonObject objective62;
		objective62[FormatSharedNts(2100034)] = 10;
		objective6[FormatSharedNts(10000)] = objective62;
		activity6.emplace_back(objective6);
		activitys.emplace_back(activity6);
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
			auto object = activity_array.at(3).get<Poseidon::JsonObject>();
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

	void WorldActivity::reload(){
		PROFILE_ME;
		auto world_activity_map = g_world_activity_map.lock();
		if(!world_activity_map){
			LOG_EMPERY_CENTER_WARNING("ActivityMap has not been loaded.");
			return;
		}
		world_activity_map->clear();
		/*
		Poseidon::OptionalMap get_params;
		get_params.set(sslit("server"), "102");
		get_params.set(sslit("channel"), "1000");
		auto world_activity_entity = SimpleHttpClientDaemon::sync_request(g_server_activity_host, g_server_activity_port, g_server_activity_use_ssl,
		Poseidon::Http::V_GET, g_server_activity_path + "/get_world_activity_info", std::move(get_params), g_server_activity_auth);
		std::istringstream world_activity_iss(world_activity_entity.dump());
		*/
		/*
		[[3510001,0,3500002,{"0":500000},1,{"guaibag10":1,"liangbag10":1,"mubag10":1,"shibag10":1}],
		[3511001,3510001,3500002,{"0":1000000},2,{"guaibag20":1,"liangbag20":1,"mubag20":1,"shibag20":1}],
		[3512001,3511001,3500002,{"2605102":1},3,{"guaibag30":1,"liangbag30":1,"mubag30":1,"shibag30":1}]]
		*/
		Poseidon::JsonArray activitys;
		// 活动1
		Poseidon::JsonArray activity1;
		activity1.emplace_back(3510001);
		activity1.emplace_back(0);
		activity1.emplace_back(3500002);
		Poseidon::JsonObject objective1;
		objective1[FormatSharedNts(0)] = 500000;
		activity1.emplace_back(objective1);
		activity1.emplace_back(1);
		Poseidon::JsonObject award1;
		award1[SharedNts("guaibag10")]  = 1;
		award1[SharedNts("liangbag10")] = 1;
		award1[SharedNts("mubag10")]    = 1;
		award1[SharedNts("shibag10")]   = 1;
		activity1.emplace_back(award1);
		activitys.emplace_back(activity1);
		// 活动2
		Poseidon::JsonArray activity2;
		activity2.emplace_back(3511001);
		activity2.emplace_back(3510001);
		activity2.emplace_back(3500002);
		Poseidon::JsonObject objective2;
		objective2[FormatSharedNts(0)] = 1000000;
		activity2.emplace_back(objective2);
		activity2.emplace_back(2);
		Poseidon::JsonObject award2;
		award2[SharedNts("guaibag20")]  = 1;
		award2[SharedNts("liangbag20")] = 1;
		award2[SharedNts("mubag20")]    = 1;
		award2[SharedNts("shibag20")]   = 1;
		activity2.emplace_back(award2);
		activitys.emplace_back(activity2);
		// 活动3
		Poseidon::JsonArray activity3;
		activity3.emplace_back(3512001);
		activity3.emplace_back(3511001);
		activity3.emplace_back(3500002);
		Poseidon::JsonObject objective3;
		objective3[FormatSharedNts(2605102)] = 1;
		activity2.emplace_back(objective3);
		activity2.emplace_back(3);
		Poseidon::JsonObject award3;
		award3[SharedNts("guaibag20")]  = 1;
		award3[SharedNts("liangbag20")] = 1;
		award3[SharedNts("mubag20")]    = 1;
		award3[SharedNts("shibag20")]   = 1;
		activity3.emplace_back(award3);
		activitys.emplace_back(activity3);

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
			auto object = activity_array.at(3).get<Poseidon::JsonObject>();
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
			object = activity_array.at(5).get<Poseidon::JsonObject>();
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
