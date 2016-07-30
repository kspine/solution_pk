#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyDungeon {

namespace Data {
	Poseidon::CsvParser sync_load_data(const char *file){
		PROFILE_ME;

		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;
		const auto path = data_directory + "/" + file + ".csv";
		LOG_EMPERY_DUNGEON_INFO("Loading csv file: path = ", path);
		csv.load(path.c_str());
		return csv;
	}
}

}
