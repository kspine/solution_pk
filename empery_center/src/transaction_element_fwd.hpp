#ifndef EMPERY_CENTER_TRANSACTION_ELEMENT_FWD_HPP_
#define EMPERY_CENTER_TRANSACTION_ELEMENT_FWD_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

template<typename FriendT, typename SomeIdT, int>
class TransactionElement;

using ItemTransactionElement           = TransactionElement<class ItemBox,       ItemId,          0>;
using ResourceTransactionElement       = TransactionElement<class Castle,        ResourceId,      1>;
using AuctionTransactionElement        = TransactionElement<class AuctionCenter, ItemId,          2>;
using SoldierTransactionElement        = TransactionElement<class Castle,        MapObjectTypeId, 3>;
using WoundedSoldierTransactionElement = TransactionElement<class Castle,        MapObjectTypeId, 4>;

}

#endif

