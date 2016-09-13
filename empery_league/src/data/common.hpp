#ifndef EMPERY_LEAGUE_DATA_COMMON_HPP_
#define EMPERY_LEAGUE_DATA_COMMON_HPP_

#include <cstddef>
#include <boost/shared_ptr.hpp>
#include <poseidon/fwd.hpp>
#include "../id_types.hpp"

namespace EmperyLeague {

namespace Data {
	extern Poseidon::CsvParser sync_load_data(const char *file);
	extern Poseidon::JsonArray encode_csv_as_json(const Poseidon::CsvParser &csv, const char *primary_key);
}

}

#endif
