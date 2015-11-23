#include "precompiled.hpp"
#include "rectangle.hpp"

namespace EmperyCenter {

std::ostream &operator<<(std::ostream &os, const Rectangle &rhs){
	return os <<"RECTANGLE" <<'[' <<rhs.bottom_left() <<',' <<rhs.top_right() <<']';
}

}
