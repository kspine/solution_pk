#ifndef EMPERY_CENTER_DATA_FORMATS_HPP_
#define EMPERY_CENTER_DATA_FORMATS_HPP_

#include <string>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include <poseidon/exception.hpp>
#include "../log.hpp"

namespace EmperyCenter {

inline Poseidon::JsonArray serialize_csv(const Poseidon::CsvParser &parser, const char *primary_key){
	Poseidon::JsonArray ret;
	const auto &raw_data = parser.get_raw_data();
	if(raw_data.empty()){
		return ret;
	}

	Poseidon::JsonArray row;

	std::ptrdiff_t primary_key_column = -1;
	for(auto it = raw_data.front().begin(); it != raw_data.front().end(); ++it){
		if(it->first == primary_key){
			if(primary_key_column != -1){
				LOG_EMPERY_CENTER_ERROR("Duplicate primary key column: primary_key = ", primary_key);
				DEBUG_THROW(Exception, Poseidon::sslit("Duplicate primary key column"));
			}
			primary_key_column = std::distance(raw_data.front().begin(), it);
		}
		row.emplace_back(it->first.get());
	}
	if(primary_key_column == -1){
		LOG_EMPERY_CENTER_ERROR("Primary key column not found: primary_key = ", primary_key);
		DEBUG_THROW(Exception, Poseidon::sslit("Primary key column not found"));
	}
	std::iter_swap(row.begin(), row.begin() + primary_key_column);
	ret.emplace_back(std::move(row));
	row.clear();

	for(auto row_it = raw_data.begin(); row_it != raw_data.end(); ++row_it){
		for(auto col_it = row_it->begin(); col_it != row_it->end(); ++col_it){
			row.emplace_back(col_it->second);
		}
		std::iter_swap(row.begin(), row.begin() + primary_key_column);
		ret.push_back(std::move(row));
		row.clear();
	}

	return ret;
}

}

#endif
