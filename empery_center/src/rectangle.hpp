#ifndef EMPERY_CENTER_RECTANGLE_HPP_
#define EMPERY_CENTER_RECTANGLE_HPP_

#include "coord.hpp"

namespace EmperyCenter {

struct Rectangle {
	Coord bl;
	Coord tr;

	constexpr Rectangle(const Coord &bl_, const Coord &tr_) noexcept
		: bl(bl_), tr((tr_.x >= bl_.x) ? tr_.x : bl_.x, (tr_.y >= bl_.y) ? tr_.y : bl_.y)
	{
	}
	constexpr Rectangle(boost::int64_t x_, boost::int64_t y_, boost::uint64_t w_, boost::uint64_t h_) noexcept
		: Rectangle(Coord(x_, y_), Coord(boost::int64_t(boost::uint64_t(x_) + w_), boost::int64_t(boost::uint64_t(y_) + h_)))
	{
	}

	constexpr boost::int64_t left() const noexcept {
		return bl.x;
	}
	constexpr boost::int64_t bottom() const noexcept {
		return bl.y;
	}
	constexpr boost::int64_t right() const noexcept {
		return tr.x;
	}
	constexpr boost::int64_t top() const noexcept {
		return tr.y;
	}

	constexpr Coord bottomLeft() const noexcept {
		return Coord(left(), bottom());
	}
	constexpr Coord bottomRight() const noexcept {
		return Coord(right(), bottom());
	}
	constexpr Coord topRight() const noexcept {
		return Coord(right(), top());
	}
	constexpr Coord topLeft() const noexcept {
		return Coord(left(), top());
	}
};

}

#endif
