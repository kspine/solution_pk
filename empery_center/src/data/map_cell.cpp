#include "../precompiled.hpp"
#include "map_cell.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include "formats.hpp"
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(TerrainMap, Data::MapCellTerrain,
		UNIQUE_MEMBER_INDEX(map_coord)
	)
	boost::weak_ptr<const TerrainMap> g_terrain_map;
	const char TERRAIN_FILE[] = "map";

	MULTI_INDEX_MAP(TicketMap, Data::MapCellTicket,
		UNIQUE_MEMBER_INDEX(ticket_item_id)
	)
	boost::weak_ptr<const TicketMap> g_ticket_map;
	const char TICKET_FILE[] = "Territory_levelup";

	MULTI_INDEX_MAP(ProductionMap, Data::MapCellProduction,
		UNIQUE_MEMBER_INDEX(terrain_id)
		MULTI_MEMBER_INDEX(best_resource_id)
	)
	boost::weak_ptr<const ProductionMap> g_production_map;
	const char PRODUCTION_MAP[] = "Territory_product";

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;
		std::string path;

		const auto terrain_map = boost::make_shared<TerrainMap>();
		path = data_directory + "/" + TERRAIN_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading map terrain: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::MapCellTerrain elem = { };

			std::string str;
			csv.get(str,             "coord");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_array(iss);
				const unsigned x = root.at(0).get<double>();
				const unsigned y = root.at(1).get<double>();
				elem.map_coord = std::make_pair(x, y);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", str = ", str);
				throw;
			}

			csv.get(elem.terrain_id, "territory_id");

			if(!terrain_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapCellTerrain: x = ", elem.map_coord.first, ", y = ", elem.map_coord.second);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellTerrain"));
			}
		}
		g_terrain_map = terrain_map;
		handles.push(DataSession::create_servlet(TERRAIN_FILE, serialize_csv(csv, "coord")));
		handles.push(terrain_map);

		const auto ticket_map = boost::make_shared<TicketMap>();
		path = data_directory + "/" + TICKET_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading map cell ticket items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::MapCellTicket elem = { };

			csv.get(elem.ticket_item_id,           "territory_certificate");
			csv.get(elem.production_rate_modifier, "output_multiple");
			csv.get(elem.capacity_modifier,        "resource_multiple");

			if(!ticket_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapCellTicket: ticket_item_id = ", elem.ticket_item_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellTicket"));
			}
		}
		g_ticket_map = ticket_map;
		handles.push(DataSession::create_servlet(TICKET_FILE, serialize_csv(csv, "territory_certificate")));
		handles.push(ticket_map);

		const auto production_map = boost::make_shared<ProductionMap>();
		path = data_directory + "/" + PRODUCTION_MAP + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading map cell production items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::MapCellProduction elem = { };

			csv.get(elem.terrain_id,           "territory_id");
			csv.get(elem.best_resource_id,     "production");

			boost::uint64_t rate = 0;
			csv.get(rate,                      "output_perminute");
			elem.best_production_rate = rate / 60000.0;

			csv.get(elem.best_capacity,        "resource_max");
			csv.get(elem.passable,             "mobile");

			if(!production_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapCellProduction: terrain_id = ", elem.terrain_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapCellProduction"));
			}
		}
		g_production_map = production_map;
		handles.push(DataSession::create_servlet(PRODUCTION_MAP, serialize_csv(csv, "territory_id")));
		handles.push(production_map);
	}
}

namespace Data {
	boost::shared_ptr<const MapCellTerrain> MapCellTerrain::get(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		const auto terrain_map = g_terrain_map.lock();
		if(!terrain_map){
			LOG_EMPERY_CENTER_WARNING("MapCellTerrainMap has not been loaded.");
			return { };
		}

		const auto it = terrain_map->find<0>(std::make_pair(map_x, map_y));
		if(it == terrain_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("MapCellterrain not found: map_x = ", map_x, ", map_y = ", map_y);
			return { };
		}
		return boost::shared_ptr<const MapCellTerrain>(terrain_map, &*it);
	}
	boost::shared_ptr<const MapCellTerrain> MapCellTerrain::require(unsigned map_x, unsigned map_y){
		PROFILE_ME;

		auto ret = get(map_x, map_y);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapCellTerrain not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapCellTicket> MapCellTicket::get(ItemId ticket_item_id){
		PROFILE_ME;

		const auto ticket_map = g_ticket_map.lock();
		if(!ticket_map){
			LOG_EMPERY_CENTER_WARNING("MapCellTicketMap has not been loaded.");
			return { };
		}

		const auto it = ticket_map->find<0>(ticket_item_id);
		if(it == ticket_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("MapCellTicket not found: ticket_item_id = ", ticket_item_id);
			return { };
		}
		return boost::shared_ptr<const MapCellTicket>(ticket_map, &*it);
	}
	boost::shared_ptr<const MapCellTicket> MapCellTicket::require(ItemId ticket_item_id){
		PROFILE_ME;

		auto ret = get(ticket_item_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapCellTicket not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapCellProduction> MapCellProduction::get(TerrainId terrain_id){
		PROFILE_ME;

		const auto production_map = g_production_map.lock();
		if(!production_map){
			LOG_EMPERY_CENTER_WARNING("MapCellProductionMap has not been loaded.");
			return { };
		}

		const auto it = production_map->find<0>(terrain_id);
		if(it == production_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("MapCellProduction not found: terrain_id = ", terrain_id);
			return { };
		}
		return boost::shared_ptr<const MapCellProduction>(production_map, &*it);
	}
	boost::shared_ptr<const MapCellProduction> MapCellProduction::require(TerrainId terrain_id){
		PROFILE_ME;

		auto ret = get(terrain_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("MapCellProduction not found"));
		}
		return ret;
	}
}

}
