#ifndef EMPERY_CENTER_SINGLETONS_GLOBAL_STATUS_HPP_
#define EMPERY_CENTER_SINGLETONS_GLOBAL_STATUS_HPP_

#include <string>
#include <boost/lexical_cast.hpp>

namespace EmperyCenter {

struct GlobalStatus {
	enum Slot {
		SLOT_INIT_SERVER_LIMIT          = 3680001,
		SLOT_INIT_SERVER_X              = 3680002,
		SLOT_INIT_SERVER_Y              = 3680003,
	};

	static const std::string &get(Slot slot);
	static void reserve(Slot slot);
	static void set(Slot slot, std::string value);

	template<typename T, typename DefaultT = T>
	static T cast(Slot slot, const DefaultT def = DefaultT()){
		const auto &str = get(slot);
		if(str.empty()){
			return def;
		}
		return boost::lexical_cast<T>(str);
	}

private:
	GlobalStatus() = delete;
};

}

#endif
