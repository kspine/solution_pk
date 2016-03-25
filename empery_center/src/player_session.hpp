#ifndef EMPERY_CENTER_PLAYER_SESSION_HPP_
#define EMPERY_CENTER_PLAYER_SESSION_HPP_

#include <string>
#include <deque>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <tuple>
#include <poseidon/cxx_util.hpp>
#include <poseidon/ip_port.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/http/low_level_session.hpp>
#include <poseidon/mutex.hpp>
#include "rectangle.hpp"

namespace EmperyCenter {

class PlayerSession : public Poseidon::Http::LowLevelSession {
public:
	enum : std::size_t {
		RED_ZONE_SIZE = 64,
	};

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

	Poseidon::Http::RequestHeaders m_request_headers;

	mutable Poseidon::Mutex m_send_queue_mutex;
	std::deque<std::tuple<unsigned, Poseidon::StreamBuffer, bool>> m_send_queue;

	Rectangle m_view;

public:
	PlayerSession(Poseidon::UniqueFile socket, std::string path);
	~PlayerSession();

protected:
	void on_close(int err_code) noexcept override;

	void on_low_level_request_headers(Poseidon::Http::RequestHeaders request_headers,
		std::string transfer_encoding, std::uint64_t content_length) override;
	void on_low_level_request_entity(std::uint64_t entity_offset, bool is_chunked, Poseidon::StreamBuffer entity) override;
	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> on_low_level_request_end(
		std::uint64_t content_length, bool is_chunked, Poseidon::OptionalMap headers) override;

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
