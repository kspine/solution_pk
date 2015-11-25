#ifndef EMPERY_CENTER_PLAYER_SESSION_HPP_
#define EMPERY_CENTER_PLAYER_SESSION_HPP_

#include <string>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <poseidon/cxx_util.hpp>
#include <poseidon/ip_port.hpp>
#include <poseidon/stream_buffer.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/http/session.hpp>

namespace EmperyCenter {

class PlayerSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		std::pair<long, std::string> (const boost::shared_ptr<PlayerSession> &session, Poseidon::StreamBuffer req)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(boost::uint16_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(boost::uint16_t message_id);

private:
	class WebSocketImpl;

private:
	const std::string m_path;

public:
	PlayerSession(Poseidon::UniqueFile socket, std::string path);
	~PlayerSession();

protected:
	void on_close(int err_code) noexcept override;

	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> predispatch_request(
		Poseidon::Http::RequestHeaders &request_headers, Poseidon::StreamBuffer &entity) override;

	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;

public:
	bool send(boost::uint16_t message_id, Poseidon::StreamBuffer payload);

	template<class MessageT>
	bool send(const MessageT &msg){
		return send(MessageT::ID, Poseidon::StreamBuffer(msg));
	}
	bool send_control(boost::uint16_t message_id, int status_code, std::string reason){
		return send(Poseidon::Cbpp::ControlMessage(message_id, static_cast<int>(status_code), std::move(reason)));
	}
	bool send_control(boost::uint16_t message_id, int status_code){
		return send_control(message_id, status_code, std::string());
	}

	void shutdown(const char *message) noexcept;
	void shutdown(int reason, const char *message) noexcept;
};

}

#endif
