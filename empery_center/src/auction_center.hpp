 #ifndef EMPERY_CENTER_AUCTION_CENTER_HPP_
#define EMPERY_CENTER_AUCTION_CENTER_HPP_

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
	class Center_AuctionTransfer;
}

class ItemBox;
class PlayerSession;

class AuctionCenter : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct ItemInfo {
		ItemId item_id;
		std::uint64_t count;
	};

	struct TransferInfo {
		MapObjectUuid map_object_uuid;
		ItemId item_id;
		std::uint64_t item_count;
		std::uint64_t item_count_locked;
		std::uint64_t item_count_fee;
		ResourceId resource_id;
		std::uint64_t resource_amount_locked;
		std::uint64_t resource_amount_fee;
		std::uint64_t created_time;
		std::uint64_t due_time;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<ItemId,
		boost::shared_ptr<MySql::Center_AuctionTransfer>> m_items;
	bool m_locked_by_transaction = false;

	boost::container::flat_map<MapObjectUuid,
		boost::container::flat_map<ItemId,
			boost::shared_ptr<MySql::Center_AuctionTransfer>>> m_transfers;

public:
	AuctionCenter(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_AuctionTransfer>> &items);
	~AuctionCenter();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	ItemInfo get_item(ItemId item_id) const;
	void get_all_items(std::vector<ItemInfo> &ret) const;

	__attribute__((__warn_unused_result__))
	ItemId commit_item_transaction_nothrow(const std::vector<AuctionTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());
	void commit_item_transaction(const std::vector<AuctionTransactionElement> &transaction,
		const boost::function<void ()> &callback = boost::function<void ()>());

	void get_transfer(std::vector<TransferInfo> &ret, MapObjectUuid map_object_uuid) const;
	void get_all_transfers(std::vector<TransferInfo> &ret) const;
	std::pair<ResourceId, ItemId> begin_transfer(MapObjectUuid map_object_uuid, const boost::shared_ptr<ItemBox> &item_box,
		const boost::container::flat_map<ItemId, std::uint64_t> &items);
	ResourceId commit_transfer(MapObjectUuid map_object_uuid);
	void cancel_transfer(MapObjectUuid map_object_uuid, const boost::shared_ptr<ItemBox> &item_box, bool refund_fee);

	void pump_transfer_status(MapObjectUuid map_object_uuid);
	void synchronize_transfer_with_player(MapObjectUuid map_object_uuid, const boost::shared_ptr<PlayerSession> &session) const;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_auction_center_with_player(const boost::shared_ptr<const AuctionCenter> &auction_center,
	const boost::shared_ptr<PlayerSession> &session)
{
	auction_center->synchronize_with_player(session);
}
inline void synchronize_auction_center_with_player(const boost::shared_ptr<AuctionCenter> &auction_center,
	const boost::shared_ptr<PlayerSession> &session)
{
	auction_center->synchronize_with_player(session);
}

}

#endif
