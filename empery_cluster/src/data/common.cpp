#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/csv_parser.hpp>

namespace EmperyCluster {

namespace Data {
	Poseidon::CsvParser sync_load_data(const char *file){
		PROFILE_ME;
/*
		const auto addr = get_config<std::string>("data_http_client_connect", "127.0.0.1");
		const auto port = get_config<unsigned>   ("data_http_client_port",    13212);
		const auto ssl  = get_config<bool>       ("data_http_client_use_ssl", false);
*/
		Poseidon::CsvParser csv;
		return csv;
	}
}

}
