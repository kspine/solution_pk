#ifndef EMPERY_CENTER_SINGLETONS_ITEM_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ITEM_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class ItemBox;

struct ItemBoxMap {
	static boost::shared_ptr<ItemBox> get(const AccountUuid &accountUuid);
	static boost::shared_ptr<ItemBox> require(const AccountUuid &accountUuid);

private:
	ItemBoxMap() = delete;
};

}

#endif
