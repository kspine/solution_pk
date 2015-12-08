#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("item/info", root, session, params){
	return Response();
}

}
