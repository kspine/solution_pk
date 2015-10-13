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
#include "log.hpp"

namespace EmperyCenter {

class PlayerSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		void (const boost::shared_ptr<PlayerSession> &session, Poseidon::StreamBuffer req)>;

public:
	static boost::shared_ptr<const ServletCallback> createServlet(boost::uint16_t messageId, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> getServlet(boost::uint16_t messageId);

private:
	class WebSocketImpl;

private:
	const std::string m_path;

public:
	PlayerSession(Poseidon::UniqueFile socket, std::string path);
	~PlayerSession();

protected:
	void onClose(int errCode) noexcept override;

	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> predispatchRequest(
		Poseidon::Http::RequestHeaders &requestHeaders, Poseidon::StreamBuffer &entity) override;
	void onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer &entity) override;

public:
	bool send(boost::uint16_t messageId, Poseidon::StreamBuffer payload);

	template<class MessageT>
	bool send(const MessageT &msg){
		LOG_EMPERY_CENTER_TRACE("Sending response to ", getRemoteInfo(), ": ", msg);
		return send(MessageT::ID, Poseidon::StreamBuffer(msg));
	}
	bool sendControl(boost::uint16_t messageId, int statusCode, std::string reason){
		return send(Poseidon::Cbpp::ControlMessage(messageId, static_cast<int>(statusCode), std::move(reason)));
	}
	bool sendControl(boost::uint16_t messageId, int statusCode){
		return sendControl(messageId, statusCode, std::string());
	}

	void shutdown(int reason, const char *message) noexcept;
};

}

#endif
