#ifndef EMPERY_LEAGUE_STRING_UTILITIES_HPP_
#define EMPERY_LEAGUE_STRING_UTILITIES_HPP_

#include <string>
#include <cstddef>

namespace EmperyLeague {

extern std::size_t hash_string_nocase(const std::string &str) noexcept;
extern bool are_strings_equal_nocase(const std::string &lhs, const std::string &rhs) noexcept;

}

#endif
