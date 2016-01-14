#include "../precompiled.hpp"
#include "vip.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(VipMap, Data::Vip,
		UNIQUE_MEMBER_INDEX(vip_level)
	)
	boost::weak_ptr<const VipMap> g_vip_map;
	const char VIP_FILE[] = "vip";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(VIP_FILE);
		const auto vip_map = boost::make_shared<VipMap>();
		while(csv.fetch_row()){
			Data::Vip elem = { };

			csv.get(elem.vip_level,        "vip_level");
			csv.get(elem.production_turbo, "vip_buff");
			csv.get(elem.max_castle_count, "castle_limit");

			if(!vip_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Vip: vip_level = ", elem.vip_level);
				DEBUG_THROW(Exception, sslit("Duplicate Vip"));
			}
		}
		g_vip_map = vip_map;
		handles.push(vip_map);
		auto servlet = DataSession::create_servlet(VIP_FILE, Data::encode_csv_as_json(csv, "vip_level"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Vip> Vip::get(unsigned vip_level){
		PROFILE_ME;

		const auto vip_map = g_vip_map.lock();
		if(!vip_map){
			LOG_EMPERY_CENTER_WARNING("VipMap has not been loaded.");
			return { };
		}

		const auto it = vip_map->find<0>(vip_level);
		if(it == vip_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("Vip not found: vip_level = ", vip_level);
			return { };
		}
		return boost::shared_ptr<const Vip>(vip_map, &*it);
	}
	boost::shared_ptr<const Vip> Vip::require(unsigned vip_level){
		PROFILE_ME;

		auto ret = get(vip_level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Vip not found"));
		}
		return ret;
	}
}

}
