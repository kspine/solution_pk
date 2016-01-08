#ifndef EMPERY_CENTER_AUCTION_CENTER_HPP_
#define EMPERY_CENTER_AUCTION_CENTER_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_AuctionTransferRequest;
	class Center_AuctionTransferRequestItem;
}

class AuctionCenter : public virtual Poseidon::VirtualSharedFromThis {
private:
	struct TransferRequest {
		boost::shared_ptr<MySql::Center_AuctionTransferRequest> obj;
		boost::container::flat_map<ItemId,
			boost::shared_ptr<MySql::Center_AuctionTransferRequestItem>> items;

		explicit TransferRequest(boost::shared_ptr<MySql::Center_AuctionTransferRequest> obj_)
			: obj(std::move(obj_))
		{
		}
	};

public:
	struct TransferRequestInfo {
		MapObjectUuid map_object_uuid;
		std::uint64_t created_time;
		std::uint64_t due_time;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<MapObjectUuid,
		TransferRequest> m_transfer_requests;

public:
	AuctionCenter(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_AuctionTransferRequest>> &transfer_requests,
		const std::vector<boost::shared_ptr<MySql::Center_AuctionTransferRequestItem>> &transfer_request_items);
	~AuctionCenter();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	TransferRequestInfo get_transfer_request(MapObjectUuid map_object_uuid) const;
	void get_all_transfer_requests(std::vector<TransferRequestInfo> &ret) const;
	void insert_transfer_request(MapObjectUuid map_object_uuid, std::uint64_t due_time,
		const boost::container::flat_map<ItemId, std::uint64_t> &resources, const boost::container::flat_map<ItemId, std::uint64_t> &items);
	void remove_transfer_request(MapObjectUuid map_object_uuid) noexcept;
};

}

#endif
