#ifndef EMPERY_CENTER_ACCOUNT_UTILITIES_HPP_
#define EMPERY_CENTER_ACCOUNT_UTILITIES_HPP_

#include <cstdint>
#include "id_types.hpp"

namespace EmperyCenter {

extern void async_accumulate_promotion_bonus(AccountUuid account_uuid, std::uint64_t taxing_amount) noexcept;

extern void async_recheck_building_level_tasks(AccountUuid account_uuid) noexcept;
extern void async_cancel_noviciate_protection(AccountUuid account_uuid) noexcept;
extern void async_recheck_tech_level_tasks(AccountUuid account_uuid) noexcept;

}

#endif
