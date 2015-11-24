#ifndef EMPERY_CLUSTER_CLUSTER_CLIENT_HPP_
#define EMPERY_CLUSTER_CLUSTER_CLIENT_HPP_

#include <poseidon/cbpp/client.hpp>
#include <poseidon/fwd.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <map>
#include <utility>
#include "id_types.hpp"
#include "log.hpp"

namespace EmperyCluster {

class ClusterClient : public Poseidon::Cbpp::Client {
public:
	using Result = std::pair<Poseidon::Cbpp::StatusCode, std::string>;

	using ServletCallback = boost::function<
		Result (const boost::shared_ptr<ClusterClient> &client, Poseidon::StreamBuffer req)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(boost::uint16_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(boost::uint16_t message_id);

	static boost::shared_ptr<ClusterClient> create(boost::int64_t numerical_x, boost::int64_t numerical_y);

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
	unsigned m_message_id;
	Poseidon::StreamBuffer m_payload;

	boost::uint64_t m_serial;
	std::multimap<boost::uint64_t, RequestElement> m_requests;

private:
	ClusterClient(const Poseidon::SockAddr &sock_addr, bool use_ssl, boost::uint64_t keep_alive_interval);

public:
	~ClusterClient();

protected:
	void on_close(int err_code) noexcept override;

	void on_sync_data_message_header(boost::uint16_t message_id, boost::uint64_t payload_size) override;
	void on_sync_data_message_payload(boost::uint64_t payload_offset, Poseidon::StreamBuffer payload) override;
	void on_sync_data_message_end(boost::uint64_t payload_size) override;

public:
	bool send(boost::uint16_t message_id, Poseidon::StreamBuffer body);
	void shutdown(const char *message) noexcept;
	void shutdown(int code, const char *message) noexcept;

	// 警告：不能在 servlet 中调用，否则会造成死锁。
	Result send_and_wait(boost::uint16_t message_id, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool send(const MsgT &msg){
		LOG_EMPERY_CLUSTER_TRACE("Sending request to center: remote = ", get_remote_info(), ", msg = ", msg);
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
	template<typename MsgT>
	Result send_and_wait(const MsgT &msg){
		LOG_EMPERY_CLUSTER_TRACE("Sending request to center: remote = ", get_remote_info(), ", msg = ", msg);
		auto ret = send_and_wait(MsgT::ID, Poseidon::StreamBuffer(msg));
		LOG_EMPERY_CLUSTER_TRACE("Received response from center: remote = ", get_remote_info(),
			", error_code = ", static_cast<int>(ret.first), ", error_message = ", ret.second);
		return ret;
	}

	bool send_notification(AccountUuid account_uuid, boost::uint16_t message_id, Poseidon::StreamBuffer body);

	template<typename MsgT>
	bool send_notification(AccountUuid account_uuid, const MsgT &msg){
		LOG_EMPERY_CLUSTER_TRACE("Sending notification to center: remote = ", get_remote_info(),
			", account_uuid = ", account_uuid, ", msg = ", msg);
		return send_notification(account_uuid, MsgT::ID, Poseidon::StreamBuffer(msg));
	}
};

}

#endif
