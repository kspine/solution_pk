#ifndef EMPERY_CENTER_TRANSACTION_ELEMENT_FWD_HPP_
#define EMPERY_CENTER_TRANSACTION_ELEMENT_FWD_HPP_

#include "id_types.hpp"

namespace EmperyCenter {

template<typename FriendT, typename SomeIdT>
class TransactionElement;

using ItemTransactionElement     = TransactionElement<class ItemBox,       ItemId>;
using ResourceTransactionElement = TransactionElement<class Castle,        ResourceId>;
using AuctionTransactionElement  = TransactionElement<class AuctionCenter, ItemId>;

}

#endif

