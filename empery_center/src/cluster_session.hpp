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
	static boost::shared_ptr<const ServletCallback> create_servlet(boost::uint16_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(boost::uint16_t message_id);

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
	void on_close(int err_code) noexcept override;

	void on_sync_data_message(boost::uint16_t message_id, Poseidon::StreamBuffer payload) override;

public:
	bool send(boost::uint16_t message_id, Poseidon::StreamBuffer body);
	void shutdown(Poseidon::Cbpp::StatusCode error_code, std::string error_message);

	// 警告：不能在 servlet 中调用，否则会造成死锁。
	Result send_and_wait(boost::uint16_t message_id, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool send(const MsgT &msg){
		LOG_EMPERY_CENTER_DEBUG("Sending request to cluster: remote = ", get_remote_info(), ", msg = ", msg);
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
	template<typename MsgT>
	Result send_and_wait(const MsgT &msg){
		LOG_EMPERY_CENTER_DEBUG("Sending request to cluster: remote = ", get_remote_info(), ", msg = ", msg);
		auto ret = send_and_wait(MsgT::ID, Poseidon::StreamBuffer(msg));
		LOG_EMPERY_CENTER_DEBUG("Received response from cluster: remote = ", get_remote_info(),
			", error_code = ", static_cast<int>(ret.first), ", error_message = ", ret.second);
		return ret;
	}
};

}

#endif
