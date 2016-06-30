#ifndef EMPERY_DUNGEON_PRECOMPILED_HPP_
#define EMPERY_DUNGEON_PRECOMPILED_HPP_

#include <poseidon/precompiled.hpp>

#include <poseidon/shared_nts.hpp>
#include <poseidon/exception.hpp>
#include <poseidon/log.hpp>
#include <poseidon/profiler.hpp>
#include <poseidon/errno.hpp>
#include <poseidon/time.hpp>
#include <poseidon/random.hpp>
#include <poseidon/flags.hpp>
#include <poseidon/module_raii.hpp>
#include <poseidon/uuid.hpp>
#include <poseidon/endian.hpp>

#include "log.hpp"
#include "checked_arithmetic.hpp"

#include <cstdint>
#include <array>
#include <type_traits>
#include <typeinfo>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace EmperyDungeon {

using Poseidon::Exception;
using Poseidon::SharedNts;

using Poseidon::sslit;

}

#endif
