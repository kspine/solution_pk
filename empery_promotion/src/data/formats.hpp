#ifndef EMPERY_PROMOTION_DATA_FORMATS_HPP_
#define EMPERY_PROMOTION_DATA_FORMATS_HPP_

#include <string>
#include <poseidon/csv_parser.hpp>
// #include <poseidon/json.hpp>
#include <poseidon/exception.hpp>
#include "../log.hpp"

namespace EmperyPromotion {
/*
inline Poseidon::JsonArray serializeCsv(const Poseidon::CsvParser &parser, const char *primaryKey){
	Poseidon::JsonArray ret;
	const AUTO_REF(rawData, parser.getRawData());
	if(!rawData.empty()){
		Poseidon::JsonArray row;

		std::ptrdiff_t primaryKeyColumn = -1;
		for(AUTO(it, rawData.front().begin()); it != rawData.front().end(); ++it){
			if(it->first == primaryKey){
				if(primaryKeyColumn != -1){
					LOG_EMPERY_PROMOTION_ERROR("Duplicate primary key column: ", primaryKey);
					DEBUG_THROW(Exception, Poseidon::sslit("Duplicate primary key column"));
				}
				primaryKeyColumn = std::distance(rawData.front().begin(), it);
			}
			row.push_back(it->first.get());
		}
		if(primaryKeyColumn == -1){
			LOG_EMPERY_PROMOTION_ERROR("Primary key column not found: ", primaryKey);
			DEBUG_THROW(Exception, Poseidon::sslit("Primary key column not found"));
		}
		std::iter_swap(row.begin(), row.begin() + primaryKeyColumn);
		ret.push_back(STD_MOVE(row));
		row.clear();

		for(AUTO(rowIt, rawData.begin()); rowIt != rawData.end(); ++rowIt){
			for(AUTO(colIt, rowIt->begin()); colIt != rowIt->end(); ++colIt){
				row.push_back(colIt->second);
			}
			std::iter_swap(row.begin(), row.begin() + primaryKeyColumn);
			ret.push_back(STD_MOVE(row));
			row.clear();
		}
	}
	return ret;
}
*/
}

#endif
