#ifndef EMPERY_PROMOTION_DATA_FORMATS_HPP_
#define EMPERY_PROMOTION_DATA_FORMATS_HPP_

#include <string>
#include <poseidon/csv_parser.hpp>
// #include <poseidon/json.hpp>
#include <poseidon/exception.hpp>
#include "../log.hpp"

namespace EmperyPromotion {
/*
inline Poseidon::JsonArray serialize_csv(const Poseidon::CsvParser &parser, const char *primary_key){
	Poseidon::JsonArray ret;
	const AUTO_REF(raw_data, parser.get_raw_data());
	if(!raw_data.empty()){
		Poseidon::JsonArray row;

		std::ptrdiff_t primary_key_column = -1;
		for(AUTO(it, raw_data.front().begin()); it != raw_data.front().end(); ++it){
			if(it->first == primary_key){
				if(primary_key_column != -1){
					LOG_EMPERY_PROMOTION_ERROR("Duplicate primary key column: ", primary_key);
					DEBUG_THROW(Exception, Poseidon::sslit("Duplicate primary key column"));
				}
				primary_key_column = std::distance(raw_data.front().begin(), it);
			}
			row.push_back(it->first.get());
		}
		if(primary_key_column == -1){
			LOG_EMPERY_PROMOTION_ERROR("Primary key column not found: ", primary_key);
			DEBUG_THROW(Exception, Poseidon::sslit("Primary key column not found"));
		}
		std::iter_swap(row.begin(), row.begin() + primary_key_column);
		ret.push_back(STD_MOVE(row));
		row.clear();

		for(AUTO(row_it, raw_data.begin()); row_it != raw_data.end(); ++row_it){
			for(AUTO(col_it, row_it->begin()); col_it != row_it->end(); ++col_it){
				row.push_back(col_it->second);
			}
			std::iter_swap(row.begin(), row.begin() + primary_key_column);
			ret.push_back(STD_MOVE(row));
			row.clear();
		}
	}
	return ret;
}
*/
}

#endif
