#include "../precompiled.hpp"
#include "signing_in.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include "formats.hpp"
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(SigningInMap, Data::SigningIn,
		UNIQUE_MEMBER_INDEX(sequential_days)
	)
	boost::weak_ptr<const SigningInMap> g_signing_in_map;
	const char SIGNING_IN_FILE[] = "sign";

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;
		std::string path;

		const auto signing_in_map = boost::make_shared<SigningInMap>();
		path = data_directory + "/" + SIGNING_IN_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading signing-in items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::SigningIn elem = { };

			csv.get(elem.sequential_days, "sign_id");
			csv.get(elem.trade_id,        "trading_id");

			if(!signing_in_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate SigningIn: sequential_days = ", elem.sequential_days);
				DEBUG_THROW(Exception, sslit("Duplicate SigningIn"));
			}
		}
		g_signing_in_map = signing_in_map;
		handles.push(DataSession::create_servlet(SIGNING_IN_FILE, serialize_csv(csv, "sign_id")));
		handles.push(signing_in_map);
	}
}

namespace Data {
	boost::shared_ptr<const SigningIn> SigningIn::get(unsigned sequential_days){
		PROFILE_ME;

		const auto signing_in_map = g_signing_in_map.lock();
		if(!signing_in_map){
			LOG_EMPERY_CENTER_WARNING("SigningInMap has not been loaded.");
			return { };
		}

		const auto it = signing_in_map->find<0>(sequential_days);
		if(it == signing_in_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("SigningIn not found: sequential_days = ", sequential_days);
			return { };
		}
		return boost::shared_ptr<const SigningIn>(signing_in_map, &*it);
	}
	boost::shared_ptr<const SigningIn> SigningIn::require(unsigned sequential_days){
		PROFILE_ME;

		auto ret = get(sequential_days);
		if(!ret){
			DEBUG_THROW(Exception, sslit("SigningIn not found"));
		}
		return ret;
	}

	unsigned SigningIn::get_max_sequential_days(){
		PROFILE_ME;

		const auto signing_in_map = g_signing_in_map.lock();
		if(!signing_in_map){
			LOG_EMPERY_CENTER_WARNING("SigningInMap has not been loaded.");
			DEBUG_THROW(Exception, sslit("SigningInMap has not been loaded"));
		}

		if(signing_in_map->empty()){
			return 0;
		}
		return signing_in_map->rbegin<0>()->sequential_days;
	}
}

}
