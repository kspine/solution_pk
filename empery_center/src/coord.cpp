#include "precompiled.hpp"
#include "coord.hpp"

namespace EmperyCenter {

std::ostream &operator<<(std::ostream &os, const Coord &rhs){
	return os <<'(' <<rhs.x() <<',' <<rhs.y() <<')';
}

}
