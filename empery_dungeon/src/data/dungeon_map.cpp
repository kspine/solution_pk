#include "../precompiled.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "dungeon_map.hpp"
namespace EmperyDungeon {

namespace {
	template<typename DungeonMapContainerT>
	void read_dungeon_map_data(DungeonMapContainerT &container,std::string dungeon_map){
		auto csv = Data::sync_load_data(dungeon_map.c_str());
		while(csv.fetch_row()){
			Data::MapCellBasic elem = { };

			Poseidon::JsonArray array;
			csv.get(array, "xy");
			const int x = array.at(0).get<double>();
			const int y = array.at(1).get<double>();
			elem.dungeon_map_coord = std::make_pair(dungeon_map,std::make_pair(x, y));

			csv.get(elem.terrain_id,         "property_id");

			if(!container->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate dungeon map CellBasic: dungeon_map = ", dungeon_map," x = ", x, ", y = ", y);
				DEBUG_THROW(Exception, sslit("Duplicate dungeon map CellBasic"));
			}
		}
	}

	MULTI_INDEX_MAP(DungeonMapContainer, Data::MapCellBasic,
		UNIQUE_MEMBER_INDEX(dungeon_map_coord)
	)
	boost::weak_ptr<const DungeonMapContainer> g_dungeon_container;

	MULTI_INDEX_MAP(TerrainContainer, Data::MapTerrain,
		UNIQUE_MEMBER_INDEX(terrain_id)
	)
	boost::weak_ptr<const TerrainContainer> g_terrain_container;
	const char TERRAIN_FILE[] = "Territory_product";

	const auto MAX_DUNGE_MAP_COUNT = 26;
	const std::string  DUNGEON_MAP_FILES[MAX_DUNGE_MAP_COUNT] = {
	"dungeon_map_0_1","dungeon_map_0_2","dungeon_map_0_3","dungeon_map_0_4","dungeon_map_0_5","dungeon_map_0_6","dungeon_map_0_7",
	"dungeon_map_1_1","dungeon_map_1_2","dungeon_map_1_3","dungeon_map_1_4","dungeon_map_1_5","dungeon_map_1_6",
	"dungeon_map_2_1","dungeon_map_2_2","dungeon_map_2_3","dungeon_map_2_4","dungeon_map_2_5","dungeon_map_2_6",
	"dungeon_map_3_1","dungeon_map_3_2","dungeon_map_3_3","dungeon_map_3_4","dungeon_map_3_5","dungeon_map_3_6",
	"dungeon_map_4_1"
	};

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto dungeon_container = boost::make_shared<DungeonMapContainer>();
		for(auto i = 0; i < MAX_DUNGE_MAP_COUNT;i++){
			read_dungeon_map_data(dungeon_container,DUNGEON_MAP_FILES[i]);
		}
		g_dungeon_container = dungeon_container;
		handles.push(dungeon_container);

		auto csv = Data::sync_load_data(TERRAIN_FILE);
		const auto terrain_container = boost::make_shared<TerrainContainer>();
		while(csv.fetch_row()){
			Data::MapTerrain elem = { };

			csv.get(elem.terrain_id,           "territory_id");

			csv.get(elem.best_resource_id,     "production");
			csv.get(elem.best_production_rate, "output_perminute");
			csv.get(elem.best_capacity,        "resource_max");
			csv.get(elem.buildable,            "construction");
			csv.get(elem.passable,             "mobile");
			csv.get(elem.protection_cost,      "need_fountain");

			if(!terrain_container->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate ContainerTerrain: terrain_id = ", elem.terrain_id);
				DEBUG_THROW(Exception, sslit("Duplicate ContainerTerrain"));
			}
		}
		g_terrain_container = terrain_container;
		handles.push(terrain_container);
	}
}

namespace Data {
	boost::shared_ptr<const MapCellBasic> MapCellBasic::get(std::string dungeon_map_name,int map_x, int map_y){
		PROFILE_ME;

		const auto dungeon_container = g_dungeon_container.lock();
		if(!dungeon_container){
			LOG_EMPERY_DUNGEON_WARNING("DungeonCellBasicContainer has not been loaded.");
			return { };
		}

		const auto it = dungeon_container->find<0>(std::make_pair(dungeon_map_name,std::make_pair(map_x, map_y)));
		if(it == dungeon_container->end<0>()){
			LOG_EMPERY_DUNGEON_TRACE("DungeonMapCellBasic not found: dungeon = ", dungeon_map_name," map_x = ", map_x, ", map_y = ", map_y);
			return { };
		}
		return boost::shared_ptr<const MapCellBasic>(dungeon_container, &*it);
	}
	boost::shared_ptr<const MapCellBasic> MapCellBasic::require(std::string dungeon_map_name,int map_x, int map_y){
		PROFILE_ME;

		auto ret = get(dungeon_map_name,map_x, map_y);
		if(!ret){
			LOG_EMPERY_DUNGEON_WARNING("DungeonMapCellBasic not found: dungeon = ", dungeon_map_name," map_x = ", map_x, ", map_y = ", map_y);
			DEBUG_THROW(Exception, sslit("DungeonMapCellBasic not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapTerrain> MapTerrain::get(TerrainId terrain_id){
		PROFILE_ME;

		const auto terrain_container = g_terrain_container.lock();
		if(!terrain_container){
			LOG_EMPERY_DUNGEON_WARNING("MapTerrainContainer has not been loaded.");
			return { };
		}

		const auto it = terrain_container->find<0>(terrain_id);
		if(it == terrain_container->end<0>()){
			LOG_EMPERY_DUNGEON_TRACE("MapTerrain not found: terrain_id = ", terrain_id);
			return { };
		}
		return boost::shared_ptr<const MapTerrain>(terrain_container, &*it);
	}
	boost::shared_ptr<const MapTerrain> MapTerrain::require(TerrainId terrain_id){
		PROFILE_ME;

		auto ret = get(terrain_id);
		if(!ret){
			LOG_EMPERY_DUNGEON_WARNING("MapTerrain not found: terrain_id = ", terrain_id);
			DEBUG_THROW(Exception, sslit("MapTerrain not found"));
		}
		return ret;
	}
}

}
