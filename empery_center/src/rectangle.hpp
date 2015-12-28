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
		: m_bl(bl)
		, m_tr((tr.x() >= bl.x()) ? tr.x() : bl.x(),
		       (tr.y() >= bl.y()) ? tr.y() : bl.y())
	{
	}
	constexpr Rectangle(std::int64_t l, boost::int64_t b, std::uint64_t w, boost::uint64_t h) noexcept
		: Rectangle(
			Coord(l, b),
			Coord(static_cast<std::int64_t>(static_cast<std::uint64_t>(l) + w),
			      static_cast<std::int64_t>(static_cast<std::uint64_t>(b) + h))
			)
	{
	}
	constexpr Rectangle(const Coord &bl, std::uint64_t w, boost::uint64_t h) noexcept
		: Rectangle(bl.x(), bl.y(), w, h)
	{
	}

public:
	constexpr std::int64_t left() const noexcept {
		return m_bl.x();
	}
	constexpr std::int64_t bottom() const noexcept {
		return m_bl.y();
	}
	constexpr std::int64_t right() const noexcept {
		return m_tr.x();
	}
	constexpr std::int64_t top() const noexcept {
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

	constexpr std::uint64_t width() const noexcept {
		return static_cast<std::uint64_t>(right() - left());
	}
	constexpr std::uint64_t height() const noexcept {
		return static_cast<std::uint64_t>(top() - bottom());
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
