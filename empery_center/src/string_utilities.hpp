#ifndef EMPERY_CENTER_STRING_UTILITIES_HPP_
#define EMPERY_CENTER_STRING_UTILITIES_HPP_

#include <string>
#include <cstddef>

namespace EmperyCenter {

extern std::size_t hash_string_nocase(const std::string &str) noexcept;
extern bool are_strings_equal_nocase(const std::string &lhs, const std::string &rhs) noexcept;

}

#endif
