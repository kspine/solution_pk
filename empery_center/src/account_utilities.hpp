#ifndef EMPERY_CENTER_ACCOUNT_UTILITIES_HPP_
#define EMPERY_CENTER_ACCOUNT_UTILITIES_HPP_

#include <cstdint>
#include "id_types.hpp"

namespace EmperyCenter {

extern void accumulate_promotion_bonus(AccountUuid account_uuid, std::uint64_t texing_amount);

}

#endif
