#ifndef EMPERY_CENTER_RECTANGLE_HPP_
#define EMPERY_CENTER_RECTANGLE_HPP_

#include "coord.hpp"

namespace EmperyCenter {

class Rectangle {
private:
	Coord m_bl;
	Coord m_tr;

public:
	constexpr Rectangle(const Coord &bl, const Coord &tr) noexcept
		: m_bl(bl), m_tr((tr.x() >= bl.x()) ? tr.x() : bl.x(), (tr.y() >= bl.y()) ? tr.y() : bl.y())
	{
	}
	constexpr Rectangle(boost::int64_t l, boost::int64_t b, boost::uint64_t w, boost::uint64_t h) noexcept
		: Rectangle(Coord(l, b),
			Coord(static_cast<boost::int64_t>(static_cast<boost::uint64_t>(l) + w),
			      static_cast<boost::int64_t>(static_cast<boost::uint64_t>(b) + h)))
	{
	}

public:
	constexpr boost::int64_t left() const noexcept {
		return m_bl.x();
	}
	constexpr boost::int64_t bottom() const noexcept {
		return m_bl.y();
	}
	constexpr boost::int64_t right() const noexcept {
		return m_tr.x();
	}
	constexpr boost::int64_t top() const noexcept {
		return m_tr.y();
	}

	constexpr Coord bottom_left() const noexcept {
		return Coord(left(), bottom());
	}
	constexpr Coord bottom_right() const noexcept {
		return Coord(right(), bottom());
	}
	constexpr Coord top_right() const noexcept {
		return Coord(right(), top());
	}
	constexpr Coord top_left() const noexcept {
		return Coord(left(), top());
	}

	constexpr bool hit_test(const Coord &coord) const noexcept {
		return (left() <= coord.x()) && (coord.x() < right()) && (bottom() <= coord.y()) && (coord.y() < top());
	}
};

inline std::ostream &operator<<(std::ostream &os, const Rectangle &rhs){
	return os <<"RECTANGLE" <<'[' <<rhs.bottom_left() <<',' <<rhs.top_right() <<']';
}

}

#endif
