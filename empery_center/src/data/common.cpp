#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyCenter {

namespace Data {
	Poseidon::CsvParser sync_load_data(const char *file){
		PROFILE_ME;

		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;
		const auto path = data_directory + "/" + file + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading csv file: path = ", path);
		csv.load(path.c_str());
		return csv;
	}
	Poseidon::JsonArray encode_csv_as_json(const Poseidon::CsvParser &csv, const char *primary_key){
		PROFILE_ME;

		Poseidon::JsonArray ret;

		const auto &data = csv.get_raw_data();

		Poseidon::JsonArray row;
		std::ptrdiff_t primary_key_column = -1;
		for(auto it = data.front().begin(); it != data.front().end(); ++it){
			if(it->first == primary_key){
				if(primary_key_column != -1){
					LOG_EMPERY_CENTER_ERROR("Duplicate primary key column: primary_key = ", primary_key);
					DEBUG_THROW(Exception, Poseidon::sslit("Duplicate primary key column"));
				}
				primary_key_column = std::distance(data.front().begin(), it);
			}
			row.emplace_back(it->first.get());
		}
		if(primary_key_column == -1){
			LOG_EMPERY_CENTER_ERROR("Primary key column not found: primary_key = ", primary_key);
			DEBUG_THROW(Exception, Poseidon::sslit("Primary key column not found"));
		}
		std::iter_swap(row.begin(), row.begin() + primary_key_column);
		ret.emplace_back(std::move(row));

		for(auto row_it = data.begin(); row_it != data.end(); ++row_it){
			row.clear();
			for(auto col_it = row_it->begin(); col_it != row_it->end(); ++col_it){
				row.emplace_back(col_it->second);
			}
			std::iter_swap(row.begin(), row.begin() + primary_key_column);
			ret.push_back(std::move(row));
		}

		return ret;
	}
}

}
