#include "../precompiled.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "common.hpp"

namespace EmperyCenter {

namespace {
	const char RANDOM_NAME[] = "random_name";
	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(RANDOM_NAME);
		auto servlet = DataSession::create_servlet(RANDOM_NAME, Data::encode_csv_as_json(csv, "number"));
		handles.push(std::move(servlet));
	}
}

}
