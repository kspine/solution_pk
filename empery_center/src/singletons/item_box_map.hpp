#ifndef EMPERY_CENTER_SINGLETONS_ITEM_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ITEM_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class ItemBox;

struct ItemBoxMap {
	static boost::shared_ptr<ItemBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<ItemBox> require(AccountUuid account_uuid);

private:
	ItemBoxMap() = delete;
};

}

#endif
