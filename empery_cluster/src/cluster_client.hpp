#ifndef EMPERY_CLUSTER_CLUSTER_CLIENT_HPP_
#define EMPERY_CLUSTER_CLUSTER_CLIENT_HPP_

#include <poseidon/cbpp/client.hpp>
#include <poseidon/fwd.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <map>
#include <utility>
#include "log.hpp"

namespace EmperyCluster {

class ClusterClient : public Poseidon::Cbpp::Client {
public:
	using ServletCallback = boost::function<
		std::pair<Poseidon::Cbpp::StatusCode, std::string> (const boost::shared_ptr<ClusterClient> &client, Poseidon::StreamBuffer req)>;

public:
	static boost::shared_ptr<const ServletCallback> createServlet(boost::uint16_t messageId, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> getServlet(boost::uint16_t messageId);

private:
	struct RequestElement {
		std::pair<Poseidon::Cbpp::StatusCode, std::string> *result;
		boost::shared_ptr<Poseidon::JobPromise> promise;

		RequestElement(std::pair<Poseidon::Cbpp::StatusCode, std::string> *result_, boost::shared_ptr<Poseidon::JobPromise> promise_)
			: result(result_), promise(std::move(promise_))
		{
		}
	};

private:
	unsigned m_messageId;
	Poseidon::StreamBuffer m_payload;

	boost::uint64_t m_serial;
	std::multimap<boost::uint64_t, RequestElement> m_requests;

public:
	explicit ClusterClient(const Poseidon::IpPort &addr, bool useSsl, boost::uint64_t keepAliveInterval);
	~ClusterClient();

protected:
	void onClose(int errCode) noexcept override;

	void onSyncDataMessageHeader(boost::uint16_t messageId, boost::uint64_t payloadSize) override;
	void onSyncDataMessagePayload(boost::uint64_t payloadOffset, Poseidon::StreamBuffer payload) override;
	void onSyncDataMessageEnd(boost::uint64_t payloadSize) override;

public:
	bool send(boost::uint16_t messageId, Poseidon::StreamBuffer body);
	void shutdown(Poseidon::Cbpp::StatusCode errorCode, std::string errorMessage);

	std::pair<Poseidon::Cbpp::StatusCode, std::string> sendAndWait(boost::uint16_t messageId, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool send(const MsgT &msg){
		LOG_EMPERY_CLUSTER_DEBUG("Sending request to cluster: remote = ", getRemoteInfo(), ", msg = ", msg);
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
	template<typename MsgT>
	std::pair<Poseidon::Cbpp::StatusCode, std::string> sendAndWait(const MsgT &msg){
		LOG_EMPERY_CLUSTER_DEBUG("Sending request to cluster: remote = ", getRemoteInfo(), ", msg = ", msg);
		auto ret = sendAndWait(MsgT::ID, Poseidon::StreamBuffer(msg));
		LOG_EMPERY_CLUSTER_DEBUG("Received response from cluster: remote = ", getRemoteInfo(),
			", errorCode = ", static_cast<int>(ret.first), ", errorMessage = ", ret.second);
		return ret;
	}
};

}

#endif
