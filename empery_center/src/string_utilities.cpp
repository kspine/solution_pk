#include "precompiled.hpp"
#include "string_utilities.hpp"

namespace EmperyCenter {

std::size_t hash_string_nocase(const std::string &str) noexcept {
	// http://www.isthe.com/chongo/tech/comp/fnv/
	std::size_t hash;
	if(sizeof(std::size_t) < 8){
		hash = 2166136261u;
		for(auto it = str.begin(); it != str.end(); ++it){
			const auto ch = std::toupper(*it);
			hash ^= static_cast<unsigned char>(ch);
			hash *= 16777619u;
		}
	} else {
		hash = 14695981039346656037u;
		for(auto it = str.begin(); it != str.end(); ++it){
			const auto ch = std::toupper(*it);
			hash ^= static_cast<unsigned char>(ch);
			hash *= 1099511628211u;
		}
	}
	return hash;
}
bool are_strings_equal_nocase(const std::string &lhs, const std::string &rhs) noexcept {
	if(lhs.size() != rhs.size()){
		return false;
	}
	for(std::size_t i = 0; i < lhs.size(); ++i){
		const auto ch1 = std::toupper(lhs[i]);
		const auto ch2 = std::toupper(rhs[i]);
		if(ch1 != ch2){
			return false;
		}
	}
	return true;
}

}
