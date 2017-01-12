#include "../precompiled.hpp"
#include "vip.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(VipContainer, Data::Vip,
		UNIQUE_MEMBER_INDEX(vip_level)
	)
	boost::weak_ptr<const VipContainer> g_vip_container;
	const char VIP_FILE[] = "vip";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(VIP_FILE);
		const auto vip_container = boost::make_shared<VipContainer>();
		while(csv.fetch_row()){
			Data::Vip elem = { };

			csv.get(elem.vip_level,        "vip_level");
			csv.get(elem.production_turbo, "vip_buff");
			csv.get(elem.max_castle_count, "immigrant_limit");
			csv.get(elem.vip_exp, "vip_exp");
			csv.get(elem.building_free_update_time, "vip_time");
			csv.get(elem.dungeon_count, "dungeon_resource");
			csv.get(elem.search_time,   "search_time");

			if(!vip_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Vip: vip_level = ", elem.vip_level);
				DEBUG_THROW(Exception, sslit("Duplicate Vip"));
			}
		}
		g_vip_container = vip_container;
		handles.push(vip_container);
		auto servlet = DataSession::create_servlet(VIP_FILE, Data::encode_csv_as_json(csv, "vip_level"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Vip> Vip::get(unsigned vip_level){
		PROFILE_ME;

		const auto vip_container = g_vip_container.lock();
		if(!vip_container){
			LOG_EMPERY_CENTER_WARNING("VipContainer has not been loaded.");
			return { };
		}

		const auto it = vip_container->find<0>(vip_level);
		if(it == vip_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("Vip not found: vip_level = ", vip_level);
			return { };
		}
		return boost::shared_ptr<const Vip>(vip_container, &*it);
	}
	boost::shared_ptr<const Vip> Vip::require(unsigned vip_level){
		PROFILE_ME;

		auto ret = get(vip_level);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("Vip not found: vip_level = ", vip_level);
			DEBUG_THROW(Exception, sslit("Vip not found"));
		}
		return ret;
	}
}

}
