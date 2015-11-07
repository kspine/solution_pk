#ifndef EMPERY_CENTER_DATA_FORMATS_HPP_
#define EMPERY_CENTER_DATA_FORMATS_HPP_

#include <string>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include <poseidon/exception.hpp>
#include "../log.hpp"

namespace EmperyCenter {

inline Poseidon::JsonArray serializeCsv(const Poseidon::CsvParser &parser, const char *primaryKey){
	Poseidon::JsonArray ret;
	const auto &rawData = parser.getRawData();
	if(rawData.empty()){
		return ret;
	}

	Poseidon::JsonArray row;

	std::ptrdiff_t primaryKeyColumn = -1;
	for(auto it = rawData.front().begin(); it != rawData.front().end(); ++it){
		if(it->first == primaryKey){
			if(primaryKeyColumn != -1){
				LOG_EMPERY_CENTER_ERROR("Duplicate primary key column: primaryKey = ", primaryKey);
				DEBUG_THROW(Exception, Poseidon::sslit("Duplicate primary key column"));
			}
			primaryKeyColumn = std::distance(rawData.front().begin(), it);
		}
		row.emplace_back(it->first.get());
	}
	if(primaryKeyColumn == -1){
		LOG_EMPERY_CENTER_ERROR("Primary key column not found: primaryKey = ", primaryKey);
		DEBUG_THROW(Exception, Poseidon::sslit("Primary key column not found"));
	}
	std::iter_swap(row.begin(), row.begin() + primaryKeyColumn);
	ret.emplace_back(std::move(row));
	row.clear();

	for(auto rowIt = rawData.begin(); rowIt != rawData.end(); ++rowIt){
		for(auto colIt = rowIt->begin(); colIt != rowIt->end(); ++colIt){
			row.emplace_back(colIt->second);
		}
		std::iter_swap(row.begin(), row.begin() + primaryKeyColumn);
		ret.push_back(std::move(row));
		row.clear();
	}

	return ret;
}

}

#endif
