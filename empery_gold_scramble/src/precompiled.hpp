#ifndef EMPERY_GOLD_SCRAMBLE_PRECOMPILED_HPP_
#define EMPERY_GOLD_SCRAMBLE_PRECOMPILED_HPP_

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
#include <poseidon/mutex.hpp>
#include <poseidon/condition_variable.hpp>
#include <poseidon/uuid.hpp>

#include "log.hpp"
#include "checked_arithmetic.hpp"

namespace EmperyGoldScramble {

using Poseidon::Exception;
using Poseidon::SharedNts;

using Poseidon::sslit;

}

#endif
