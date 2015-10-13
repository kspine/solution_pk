#include "precompiled.hpp"
#include "player_session.hpp"
#include <poseidon/job_base.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/cbpp/control_codes.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/websocket/session.hpp>
#include <poseidon/websocket/status_codes.hpp>
#include <poseidon/websocket/exception.hpp>
#include <poseidon/websocket/handshake.hpp>
#include "singletons/player_session_map.hpp"
#include "msg/kill.hpp"

namespace EmperyCenter {

using ServletCallback = PlayerSession::ServletCallback;

namespace {
	class OnCloseJob : public Poseidon::JobBase {
	private:
		const boost::weak_ptr<PlayerSession> m_session;
		const int m_errCode;

	public:
		OnCloseJob(boost::weak_ptr<PlayerSession> session, int errCode)
			: m_session(std::move(session)), m_errCode(errCode)
		{
		}

	protected:
		boost::weak_ptr<const void> getCategory() const override {
			return m_session;
		}
		void perform() const override {
			PROFILE_ME;

			PlayerSessionMap::remove(m_session); // noexcept
		}
	};

	std::map<unsigned, boost::weak_ptr<const ServletCallback> > g_servletMap;
}

class PlayerSession::WebSocketImpl : public Poseidon::WebSocket::Session {
public:
	explicit WebSocketImpl(const boost::shared_ptr<PlayerSession> &parent)
		: Poseidon::WebSocket::Session(parent)
	{
	}

protected:
	void onSyncDataMessage(Poseidon::WebSocket::OpCode opcode, const Poseidon::StreamBuffer &payload) override {
		PROFILE_ME;

		const auto parent = boost::dynamic_pointer_cast<PlayerSession>(getParent());
		if(!parent){
			forceShutdown();
			return;
		}

		unsigned messageId = UINT_MAX;
		try {
			if(opcode != Poseidon::WebSocket::OP_DATA_BIN){
				LOG_EMPERY_CENTER_WARNING("Invalid message type: opcode = ", opcode);
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			auto cbppMessage = payload;
			auto read = Poseidon::StreamBuffer::ReadIterator(cbppMessage);
			boost::uint64_t messageId64;
			if(!Poseidon::vuint50FromBinary(messageId64, read, payload.size())){
				LOG_EMPERY_CENTER_WARNING("Packet too small");
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			messageId = messageId64 & 0xFFFF;
			if(messageId != messageId64){
				LOG_EMPERY_CENTER_WARNING("Message id too large: messageId64 = ", messageId64);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
			}

			const auto servlet = PlayerSession::getServlet(messageId);
			if(!servlet){
				LOG_EMPERY_CENTER_WARNING("No servlet found: messageId = ", messageId);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
			}
			(*servlet)(parent, std::move(cbppMessage));
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: messageId = ", messageId, ", what = ", e.what());
			if(e.statusCode() < 0){
				parent->shutdown(e.statusCode(), e.what());
			} else {
				parent->sendControl(messageId, e.statusCode(), e.what());
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: messageId = ", messageId, ", what = ", e.what());
			parent->shutdown(Msg::ST_INTERNAL_ERROR, e.what());
		}
	}
};

boost::shared_ptr<const ServletCallback> PlayerSession::createServlet(boost::uint16_t messageId, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	auto &weakServlet = g_servletMap[messageId];
	if(!weakServlet.expired()){
		LOG_EMPERY_CENTER_ERROR("Duplicate player servlet: messageId = ", messageId);
		DEBUG_THROW(Exception, sslit("Duplicate player servlet"));
	}
	weakServlet = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> PlayerSession::getServlet(boost::uint16_t messageId){
	PROFILE_ME;

	const auto it = g_servletMap.find(messageId);
	if(it == g_servletMap.end()){
		return { };
	}
	return it->second.lock();
}

PlayerSession::PlayerSession(Poseidon::UniqueFile socket, std::string path)
	: Poseidon::Http::Session(std::move(socket))
	, m_path(std::move(path))
{
}
PlayerSession::~PlayerSession(){
}

void PlayerSession::onClose(int errCode) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CENTER_DEBUG("Socket close: errCode = ", errCode, ", description = ", Poseidon::getErrorDesc(errCode));

	try {
		Poseidon::enqueueJob(boost::make_shared<OnCloseJob>(virtualWeakFromThis<PlayerSession>(), errCode));
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
	}

	Poseidon::Http::Session::onClose(errCode);
}

boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> PlayerSession::predispatchRequest(
	Poseidon::Http::RequestHeaders &requestHeaders, Poseidon::StreamBuffer &entity)
{
	PROFILE_ME;

	if(requestHeaders.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_IMPLEMENTED);
	}
	auto uri = Poseidon::Http::urlDecode(requestHeaders.uri);
	if(uri != m_path){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	if(::strcasecmp(requestHeaders.headers.get("Upgrade").c_str(), "websocket") == 0){
		auto upgradedSession = boost::make_shared<WebSocketImpl>(virtualSharedFromThis<PlayerSession>());

		auto responseHeaders = Poseidon::WebSocket::makeHandshakeResponse(requestHeaders);
		if(responseHeaders.statusCode != Poseidon::Http::ST_SWITCHING_PROTOCOLS){
			DEBUG_THROW(Poseidon::Http::Exception, responseHeaders.statusCode);
		}
		Poseidon::Http::Session::send(std::move(responseHeaders), { });

		return std::move(upgradedSession);
	}

	return Poseidon::Http::Session::predispatchRequest(requestHeaders, entity);
}
void PlayerSession::onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer &entity){
	PROFILE_ME;

	(void)requestHeaders;
	(void)entity;

	DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
}

bool PlayerSession::send(boost::uint16_t messageId, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(getUpgradedSession());
	if(!impl){
		LOG_EMPERY_CENTER_WARNING("WebSocket session comes out of thin air?");
		DEBUG_THROW(Exception, sslit("WebSocket session comes out of thin air"));
	}

	Poseidon::StreamBuffer whole;
	auto wit = Poseidon::StreamBuffer::WriteIterator(whole);
	Poseidon::vuint50ToBinary(messageId, wit);
	whole.splice(payload);
	return impl->send(std::move(whole), true);
}

void PlayerSession::shutdown(int reason, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		sendControl(Poseidon::Cbpp::ControlMessage::ID, reason, message);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}
	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(getUpgradedSession());
	if(impl){
		impl->shutdown(Poseidon::WebSocket::ST_NORMAL_CLOSURE);
	}

	Poseidon::Http::Session::shutdownRead();
	Poseidon::Http::Session::shutdownWrite();
}

}
