#ifndef EMPERY_CENTER_ITEM_BOX_HPP_
#define EMPERY_CENTER_ITEM_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "id_types.hpp"
#include "transaction_element_fwd.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Item;
}

class PlayerSession;

class ItemBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct ItemInfo {
		ItemId item_id;
		std::uint64_t count;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<ItemId,
		boost::shared_ptr<MySql::Center_Item>> m_items;
	bool m_locked_by_transaction = false;

public:
	ItemBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Item>> &items);
	~ItemBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void check_init_items();

	ItemInfo get(ItemId item_id) const;
	void get_all(std::vector<ItemInfo> &ret) const;

	__attribute__((__warn_unused_result__))
	ItemId commit_transaction_nothrow(const std::vector<ItemTransactionElement> &transaction, bool tax,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_transaction(const std::vector<ItemTransactionElement> &transaction, bool tax,
		const boost::function<void ()> &callback = boost::function<void ()>());

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_item_box_with_player(const boost::shared_ptr<const ItemBox> &item_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	item_box->synchronize_with_player(session);
}
inline void synchronize_item_box_with_player(const boost::shared_ptr<ItemBox> &item_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	item_box->synchronize_with_player(session);
}

}

#endif

