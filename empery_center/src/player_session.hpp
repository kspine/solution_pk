#ifndef EMPERY_CENTER_PLAYER_SESSION_HPP_
#define EMPERY_CENTER_PLAYER_SESSION_HPP_

#include <string>
#include <deque>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <poseidon/cxx_util.hpp>
#include <poseidon/ip_port.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/mutex.hpp>
#include "rectangle.hpp"

namespace EmperyCenter {

class PlayerSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		std::pair<long, std::string> (const boost::shared_ptr<PlayerSession> &, Poseidon::StreamBuffer)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(std::uint16_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(std::uint16_t message_id);

private:
	class WebSocketImpl;
	class QueueImpl;

private:
	const std::string m_path;

	mutable Poseidon::Mutex m_send_queue_mutex;
	std::deque<std::pair<unsigned, Poseidon::StreamBuffer>> m_send_queue;

	Rectangle m_view;

public:
	PlayerSession(Poseidon::UniqueFile socket, std::string path);
	~PlayerSession();

protected:
	void on_close(int err_code) noexcept override;

	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> predispatch_request(
		Poseidon::Http::RequestHeaders &request_headers, Poseidon::StreamBuffer &entity) override;

	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;

public:
	bool send(std::uint16_t message_id, Poseidon::StreamBuffer payload);
	template<class MessageT>
	bool send(const MessageT &msg){
		return send(MessageT::ID, Poseidon::StreamBuffer(msg));
	}
	bool send_control(std::uint16_t message_id, int status_code, std::string reason);
	bool send_control(std::uint16_t message_id, int status_code){
		return send_control(message_id, status_code, std::string());
	}

	void shutdown(const char *message) noexcept;
	void shutdown(int reason, const char *message) noexcept;

	Rectangle get_view() const {
		return m_view;
	}
	void set_view(Rectangle view);
};

}

#endif
