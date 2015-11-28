#include "../precompiled.hpp"
#include "map_cell.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include "formats.hpp"
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
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
			csv.get(elem.best_production_rate, "output_perminute");
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

	bool MapCellProduction::is_resource_producible(ResourceId resource_id){
		PROFILE_ME;

		const auto production_map = g_production_map.lock();
		if(!production_map){
			LOG_EMPERY_CENTER_WARNING("MapCellProductionMap has not been loaded.");
			DEBUG_THROW(Exception, sslit("MapCellProductionMap has not been loaded"));
		}

		const auto it = production_map->find<1>(resource_id);
		if(it == production_map->end<1>()){
			return false;
		}
		return true;
	}
}

}
