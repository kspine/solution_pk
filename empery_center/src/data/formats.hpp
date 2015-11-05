#ifndef EMPERY_CENTER_DATA_FORMATS_HPP_
#define EMPERY_CENTER_DATA_FORMATS_HPP_

#include <string>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include <poseidon/exception.hpp>
#include "../log.hpp"

namespace EmperyCenter {

inline Poseidon::JsonArray serializeCsv(const Poseidon::CsvParser &parser){
	Poseidon::JsonArray ret;
	const AUTO_REF(rawData, parser.getRawData());
	if(!rawData.empty()){
		Poseidon::JsonArray row;

		for(AUTO(it, rawData.front().begin()); it != rawData.front().end(); ++it){
			row.push_back(it->first.get());
		}
		ret.push_back(STD_MOVE(row));
		row.clear();

		for(AUTO(rowIt, rawData.begin()); rowIt != rawData.end(); ++rowIt){
			for(AUTO(colIt, rowIt->begin()); colIt != rowIt->end(); ++colIt){
				row.push_back(colIt->second);
			}
			ret.push_back(STD_MOVE(row));
			row.clear();
		}
	}
	return ret;
}

}

#endif
