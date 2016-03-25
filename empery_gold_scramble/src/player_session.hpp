#ifndef EMPERY_GOLD_SCRAMBLE_PLAYER_SESSION_HPP_
#define EMPERY_GOLD_SCRAMBLE_PLAYER_SESSION_HPP_

#include <string>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdint>
#include <poseidon/fwd.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/ip_port.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/http/session.hpp>
#include "log.hpp"

namespace EmperyGoldScramble {

class PlayerSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		std::pair<long, std::string> (const boost::shared_ptr<PlayerSession> &session, Poseidon::StreamBuffer req)>;
	using HttpServletCallback = boost::function<
		Poseidon::JsonObject (const boost::shared_ptr<PlayerSession> &session, const Poseidon::OptionalMap &params)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(std::uint16_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(std::uint16_t message_id);

	static boost::shared_ptr<const HttpServletCallback> create_http_servlet(const std::string &uri, HttpServletCallback callback);
	static boost::shared_ptr<const HttpServletCallback> get_http_servlet(const std::string &uri);

private:
	class WebSocketImpl;

private:
	const std::string m_path;

public:
	PlayerSession(Poseidon::UniqueFile socket, std::string path);
	~PlayerSession();

protected:
	void on_close(int err_code) noexcept override;

	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> on_low_level_request_end(
		std::uint64_t content_length, bool is_chunked, Poseidon::OptionalMap headers) override;

	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;

public:
	bool send(std::uint16_t message_id, Poseidon::StreamBuffer payload);

	template<class MessageT>
	bool send(const MessageT &msg){
		if(MessageT::ID != 0){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Sending data to ", get_remote_info(), ": ", msg);
		}
		return send(MessageT::ID, Poseidon::StreamBuffer(msg));
	}
	bool send_control(std::uint16_t message_id, int status_code, std::string reason){
		return send(Poseidon::Cbpp::ControlMessage(message_id, static_cast<int>(status_code), std::move(reason)));
	}
	bool send_control(std::uint16_t message_id, int status_code){
		return send_control(message_id, status_code, std::string());
	}

	void shutdown(int reason, const char *message) noexcept;
};

}

#endif
