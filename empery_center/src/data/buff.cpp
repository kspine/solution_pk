#include "../precompiled.hpp"
#include "buff.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(BuffMap, Data::Buff,
		UNIQUE_MEMBER_INDEX(buff_id)
	)
	boost::weak_ptr<const BuffMap> g_buff_map;
	const char BUFF_FILE[] = "BUFF";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BUFF_FILE);
		const auto buff_map = boost::make_shared<BuffMap>();
		while(csv.fetch_row()){
			Data::Buff elem = { };

			csv.get(elem.buff_id, "buff_id");

			Poseidon::JsonObject object;
			csv.get(object, "numerical");
			elem.attributes.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto attribute_id = boost::lexical_cast<AttributeId>(it->first);
				const auto value = it->second.get<double>();
				if(!elem.attributes.emplace(attribute_id, value).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate attribute: attribute_id = ", attribute_id);
					DEBUG_THROW(Exception, sslit("Duplicate attribute"));
				}
			}

			if(!buff_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Buff: buff_id = ", elem.buff_id);
				DEBUG_THROW(Exception, sslit("Duplicate Buff"));
			}
		}
		g_buff_map = buff_map;
		handles.push(buff_map);
		auto servlet = DataSession::create_servlet(BUFF_FILE, Data::encode_csv_as_json(csv, "sign_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Buff> Buff::get(BuffId buff_id){
		PROFILE_ME;

		const auto buff_map = g_buff_map.lock();
		if(!buff_map){
			LOG_EMPERY_CENTER_WARNING("BuffMap has not been loaded.");
			return { };
		}

		const auto it = buff_map->find<0>(buff_id);
		if(it == buff_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("Buff not found: buff_id = ", buff_id);
			return { };
		}
		return boost::shared_ptr<const Buff>(buff_map, &*it);
	}
	boost::shared_ptr<const Buff> Buff::require(BuffId buff_id){
		PROFILE_ME;

		auto ret = get(buff_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("Buff not found: buff_id = ", buff_id);
			DEBUG_THROW(Exception, sslit("Buff not found"));
		}
		return ret;
	}
}

}
