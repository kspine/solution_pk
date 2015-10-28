#ifndef EMPERY_CENTER_COORD_HPP_
#define EMPERY_CENTER_COORD_HPP_

#include <boost/cstdint.hpp>

namespace EmperyCenter {

struct Coord {
	boost::int64_t x;
	boost::int64_t y;

	constexpr Coord(boost::int64_t x_, boost::int64_t y_) noexcept
		: x(x_), y(y_)
	{
	}

	bool operator<(const Coord &rhs) const noexcept {
		if(x != rhs.x){
			return x < rhs.x;
		}
		return y < rhs.y;
	}
};

}

#endif
