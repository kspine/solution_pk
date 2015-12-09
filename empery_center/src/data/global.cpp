#include "../precompiled.hpp"
#include "global.hpp"
#include <boost/container/flat_map.hpp>
#include "formats.hpp"
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	struct DataStorage {
		std::string str;

		mutable std::pair<bool, boost::int64_t>       i64;
		mutable std::pair<bool, boost::uint64_t>      u64;
		mutable std::pair<bool, double>               dbl;
		mutable std::pair<bool, Poseidon::JsonArray>  arr;
		mutable std::pair<bool, Poseidon::JsonObject> obj;
	};

	using GlobalMap = boost::container::flat_map<Data::Global::Slot, DataStorage>;
	boost::weak_ptr<const GlobalMap> g_global_map;
	const char GLOBAL_FILE[] = "Public";

	MODULE_RAII_PRIORITY(handles, 900){
		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;
		std::string path;
		boost::shared_ptr<const DataSession::SerializedData> servlet;

		const auto global_map = boost::make_shared<GlobalMap>();
		path = data_directory + "/" + GLOBAL_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading global config: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			unsigned slot;
			DataStorage storage = { };

			csv.get(slot,        "id");
			csv.get(storage.str, "numerical");

			if(!global_map->emplace((Data::Global::Slot)slot, std::move(storage)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate global config: slot = ", slot);
				DEBUG_THROW(Exception, sslit("Duplicate global config"));
			}
		}
		g_global_map = global_map;
		handles.push(global_map);
		servlet = DataSession::create_servlet(GLOBAL_FILE, serialize_csv(csv, "id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	const std::string &Global::as_string(Slot slot){
		PROFILE_ME;

		const auto global_map = g_global_map.lock();
		if(!global_map){
			LOG_EMPERY_CENTER_WARNING("Global config map has not been loaded.");
			DEBUG_THROW(Exception, sslit("Global config map has not been loaded"));
		}

		const auto it = global_map->find(slot);
		if(it == global_map->end()){
			LOG_EMPERY_CENTER_WARNING("Global config not found: slot = ", slot);
			DEBUG_THROW(Exception, sslit("Global confignot found"));
		}
		return it->second.str;
	}
	boost::int64_t Global::as_signed(Slot slot){
		PROFILE_ME;

		const auto global_map = g_global_map.lock();
		if(!global_map){
			LOG_EMPERY_CENTER_WARNING("Global config map has not been loaded.");
			DEBUG_THROW(Exception, sslit("Global config map has not been loaded"));
		}

		const auto it = global_map->find(slot);
		if(it == global_map->end()){
			LOG_EMPERY_CENTER_WARNING("Global config not found: slot = ", slot);
			DEBUG_THROW(Exception, sslit("Global confignot found"));
		}
		if(!it->second.i64.first){
			const auto val = boost::lexical_cast<boost::int64_t>(it->second.str);
			it->second.i64.second = val;
			it->second.i64.first = true;
		}
		return it->second.i64.second;
	}
	boost::uint64_t Global::as_unsigned(Slot slot){
		PROFILE_ME;

		const auto global_map = g_global_map.lock();
		if(!global_map){
			LOG_EMPERY_CENTER_WARNING("Global config map has not been loaded.");
			DEBUG_THROW(Exception, sslit("Global config map has not been loaded"));
		}

		const auto it = global_map->find(slot);
		if(it == global_map->end()){
			LOG_EMPERY_CENTER_WARNING("Global config not found: slot = ", slot);
			DEBUG_THROW(Exception, sslit("Global confignot found"));
		}
		if(!it->second.u64.first){
			const auto val = boost::lexical_cast<boost::uint64_t>(it->second.str);
			it->second.u64.second = val;
			it->second.u64.first = true;
		}
		return it->second.u64.second;
	}
	double Global::as_double(Slot slot){
		PROFILE_ME;

		const auto global_map = g_global_map.lock();
		if(!global_map){
			LOG_EMPERY_CENTER_WARNING("Global config map has not been loaded.");
			DEBUG_THROW(Exception, sslit("Global config map has not been loaded"));
		}

		const auto it = global_map->find(slot);
		if(it == global_map->end()){
			LOG_EMPERY_CENTER_WARNING("Global config not found: slot = ", slot);
			DEBUG_THROW(Exception, sslit("Global confignot found"));
		}
		if(!it->second.dbl.first){
			const auto val = boost::lexical_cast<double>(it->second.str);
			it->second.dbl.second = val;
			it->second.dbl.first = true;
		}
		return it->second.dbl.second;
	}
	const Poseidon::JsonArray &Global::as_array(Slot slot){
		PROFILE_ME;

		const auto global_map = g_global_map.lock();
		if(!global_map){
			LOG_EMPERY_CENTER_WARNING("Global config map has not been loaded.");
			DEBUG_THROW(Exception, sslit("Global config map has not been loaded"));
		}

		const auto it = global_map->find(slot);
		if(it == global_map->end()){
			LOG_EMPERY_CENTER_WARNING("Global config not found: slot = ", slot);
			DEBUG_THROW(Exception, sslit("Global confignot found"));
		}
		if(!it->second.arr.first){
			std::istringstream iss(it->second.str);
			auto val = Poseidon::JsonParser::parse_array(iss);
			it->second.arr.second = std::move(val);
			it->second.arr.first = true;
		}
		return it->second.arr.second;
	}
	const Poseidon::JsonObject &Global::as_object(Slot slot){
		PROFILE_ME;

		const auto global_map = g_global_map.lock();
		if(!global_map){
			LOG_EMPERY_CENTER_WARNING("Global config map has not been loaded.");
			DEBUG_THROW(Exception, sslit("Global config map has not been loaded"));
		}

		const auto it = global_map->find(slot);
		if(it == global_map->end()){
			LOG_EMPERY_CENTER_WARNING("Global config not found: slot = ", slot);
			DEBUG_THROW(Exception, sslit("Global confignot found"));
		}
		if(!it->second.obj.first){
			std::istringstream iss(it->second.str);
			auto val = Poseidon::JsonParser::parse_object(iss);
			it->second.obj.second = std::move(val);
			it->second.obj.first = true;
		}
		return it->second.obj.second;
	}
}

}
