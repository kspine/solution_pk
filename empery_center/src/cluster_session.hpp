#ifndef EMPERY_CENTER_CLUSTER_SESSION_HPP_
#define EMPERY_CENTER_CLUSTER_SESSION_HPP_

#include <poseidon/cbpp/session.hpp>
#include <poseidon/fwd.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <map>
#include <utility>
#include "log.hpp"

namespace EmperyCenter {

class ClusterSession : public Poseidon::Cbpp::Session {
public:
	using Result = std::pair<Poseidon::Cbpp::StatusCode, std::string>;

	using ServletCallback = boost::function<
		Result (const boost::shared_ptr<ClusterSession> &session, Poseidon::StreamBuffer req)>;

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
	boost::uint64_t m_serial;
	std::multimap<boost::uint64_t, RequestElement> m_requests;

public:
	explicit ClusterSession(Poseidon::UniqueFile socket);
	~ClusterSession();

protected:
	void onClose(int errCode) noexcept override;

	void onSyncDataMessage(boost::uint16_t messageId, Poseidon::StreamBuffer payload) override;

public:
	bool send(boost::uint16_t messageId, Poseidon::StreamBuffer body);
	void shutdown(Poseidon::Cbpp::StatusCode errorCode, std::string errorMessage);

	// 警告：不能在 servlet 中调用，否则会造成死锁。
	Result sendAndWait(boost::uint16_t messageId, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool send(const MsgT &msg){
		LOG_EMPERY_CENTER_DEBUG("Sending request to cluster: remote = ", getRemoteInfo(), ", msg = ", msg);
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
	template<typename MsgT>
	Result sendAndWait(const MsgT &msg){
		LOG_EMPERY_CENTER_DEBUG("Sending request to cluster: remote = ", getRemoteInfo(), ", msg = ", msg);
		auto ret = sendAndWait(MsgT::ID, Poseidon::StreamBuffer(msg));
		LOG_EMPERY_CENTER_DEBUG("Received response from cluster: remote = ", getRemoteInfo(),
			", errorCode = ", static_cast<int>(ret.first), ", errorMessage = ", ret.second);
		return ret;
	}
};

}

#endif
