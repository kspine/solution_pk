#ifndef EMPERY_CLUSTER_CLUSTER_CLIENT_HPP_
#define EMPERY_CLUSTER_CLUSTER_CLIENT_HPP_

#include <poseidon/cbpp/client.hpp>
#include <poseidon/fwd.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <map>
#include <utility>
#include "log.hpp"
#include "../../empery_center/src/id_types.hpp"

namespace EmperyCluster {

class ClusterClient : public Poseidon::Cbpp::Client {
public:
	using Result = std::pair<Poseidon::Cbpp::StatusCode, std::string>;

	using ServletCallback = boost::function<
		Result (const boost::shared_ptr<ClusterClient> &client, Poseidon::StreamBuffer req)>;

public:
	static boost::shared_ptr<const ServletCallback> createServlet(boost::uint16_t messageId, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> getServlet(boost::uint16_t messageId);

private:
	struct RequestElement {
		Result *result;
		boost::shared_ptr<Poseidon::JobPromise> promise;

		RequestElement(Result *result_, boost::shared_ptr<Poseidon::JobPromise> promise_)
			: result(result_), promise(std::move(promise_))
		{
		}
	};

private:
	unsigned m_messageId;
	Poseidon::StreamBuffer m_payload;

	const boost::int64_t m_mapX;
	const boost::int64_t m_mapY;

	boost::uint64_t m_serial;
	std::multimap<boost::uint64_t, RequestElement> m_requests;

public:
	explicit ClusterClient(const Poseidon::IpPort &addr, bool useSsl, boost::uint64_t keepAliveInterval,
		boost::int64_t mapX, boost::int64_t mapY);
	~ClusterClient();

protected:
	void onConnect() override;
	void onClose(int errCode) noexcept override;

	void onSyncDataMessageHeader(boost::uint16_t messageId, boost::uint64_t payloadSize) override;
	void onSyncDataMessagePayload(boost::uint64_t payloadOffset, Poseidon::StreamBuffer payload) override;
	void onSyncDataMessageEnd(boost::uint64_t payloadSize) override;

public:
	bool send(boost::uint16_t messageId, Poseidon::StreamBuffer body);
	void shutdown(Poseidon::Cbpp::StatusCode errorCode, std::string errorMessage);

	// 警告：不能在 servlet 中调用，否则会造成死锁。
	Result sendAndWait(boost::uint16_t messageId, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool send(const MsgT &msg){
		LOG_EMPERY_CLUSTER_DEBUG("Sending request to center: remote = ", getRemoteInfo(), ", msg = ", msg);
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
	template<typename MsgT>
	Result sendAndWait(const MsgT &msg){
		LOG_EMPERY_CLUSTER_DEBUG("Sending request to center: remote = ", getRemoteInfo(), ", msg = ", msg);
		auto ret = sendAndWait(MsgT::ID, Poseidon::StreamBuffer(msg));
		LOG_EMPERY_CLUSTER_DEBUG("Received response from center: remote = ", getRemoteInfo(),
			", errorCode = ", static_cast<int>(ret.first), ", errorMessage = ", ret.second);
		return ret;
	}

	bool sendNotification(const EmperyCenter::AccountUuid &accountUuid, boost::uint16_t messageId, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool sendNotification(const EmperyCenter::AccountUuid &accountUuid, const MsgT &msg){
		LOG_EMPERY_CLUSTER_DEBUG("Sending notification to center: remote = ", getRemoteInfo(),
			", accountUuid = ", accountUuid, ", msg = ", msg);
		return sendNotification(accountUuid, MsgT::ID, Poseidon::StreamBuffer(msg));
	}
};

}

#endif
