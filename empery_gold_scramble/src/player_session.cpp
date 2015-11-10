#include "precompiled.hpp"
#include "player_session.hpp"
#include <poseidon/async_job.hpp>
#include <poseidon/json.hpp>
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

namespace EmperyGoldScramble {

using ServletCallback     = PlayerSession::ServletCallback;
using HttpServletCallback = PlayerSession::HttpServletCallback;

namespace {
	std::map<unsigned, boost::weak_ptr<const ServletCallback>>        g_servletMap;
	std::map<std::string, boost::weak_ptr<const HttpServletCallback>> g_httpServletMap;
}

class PlayerSession::WebSocketImpl : public Poseidon::WebSocket::Session {
public:
	explicit WebSocketImpl(const boost::shared_ptr<PlayerSession> &parent)
		: Poseidon::WebSocket::Session(parent)
	{
	}

protected:
	void onSyncDataMessage(Poseidon::WebSocket::OpCode opcode, Poseidon::StreamBuffer payload) override {
		PROFILE_ME;

		const auto parent = boost::dynamic_pointer_cast<PlayerSession>(getParent());
		if(!parent){
			forceShutdown();
			return;
		}

		unsigned messageId = UINT_MAX;
		std::pair<long, std::string> result;
		try {
			if(opcode != Poseidon::WebSocket::OP_DATA_BIN){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Invalid message type: opcode = ", opcode);
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			auto read = Poseidon::StreamBuffer::ReadIterator(payload);
			boost::uint64_t messageId64;
			if(!Poseidon::vuint50FromBinary(messageId64, read, payload.size())){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Packet too small");
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			messageId = messageId64 & 0xFFFF;
			if(messageId != messageId64){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Message id too large: messageId64 = ", messageId64);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
			}

			if(messageId == Poseidon::Cbpp::ControlMessage::ID){
				const auto req = Poseidon::Cbpp::ControlMessage(std::move(payload));
				LOG_EMPERY_GOLD_SCRAMBLE_TRACE("Received ping from ", parent->getRemoteInfo(), ": req = ", req);
				result.first = Poseidon::Cbpp::ST_PONG;
				result.second = std::move(req.stringParam);
			} else {
				const auto servlet = PlayerSession::getServlet(messageId);
				if(!servlet){
					LOG_EMPERY_GOLD_SCRAMBLE_WARNING("No servlet found: messageId = ", messageId);
					DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
				}
				result = (*servlet)(parent, std::move(payload));
			}
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: messageId = ", messageId, ", what = ", e.what());
			result.first = e.statusCode();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: messageId = ", messageId, ", what = ", e.what());
			result.first = Msg::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		if(messageId != Poseidon::Cbpp::ControlMessage::ID){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Sending response: messageId = ", messageId,
				", code = ", result.first, ", message = ", result.second);
		}
		if(result.first < 0){
			parent->shutdown(result.first, result.second.c_str());
		} else {
			parent->sendControl(messageId, result.first, std::move(result.second));
		}
	}
};

boost::shared_ptr<const ServletCallback> PlayerSession::createServlet(boost::uint16_t messageId, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	auto &weakServlet = g_servletMap[messageId];
	if(!weakServlet.expired()){
		LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Duplicate player servlet: messageId = ", messageId);
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
	auto servlet = it->second.lock();
	if(!servlet){
		g_servletMap.erase(it);
		return { };
	}
	return servlet;
}

boost::shared_ptr<const HttpServletCallback> PlayerSession::createHttpServlet(const std::string &uri, HttpServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<HttpServletCallback>(std::move(callback));
	if(!g_httpServletMap.insert(std::make_pair(uri, servlet)).second){
		LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Duplicate player HTTP servlet: uri = ", uri);
		DEBUG_THROW(Exception, sslit("Duplicate player HTTP servlet"));
	}
	return std::move(servlet);
}
boost::shared_ptr<const HttpServletCallback> PlayerSession::getHttpServlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_httpServletMap.find(uri);
	if(it == g_httpServletMap.end()){
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
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Socket close: errCode = ", errCode, ", description = ", Poseidon::getErrorDesc(errCode));

	try {
		Poseidon::enqueueAsyncJob(virtualWeakFromThis<PlayerSession>(),
			boost::bind(&PlayerSessionMap::remove, virtualWeakFromThis<PlayerSession>()));
	} catch(std::exception &e){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
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
	if(uri == m_path){
		if(::strcasecmp(requestHeaders.headers.get("Upgrade").c_str(), "websocket") == 0){
			auto upgradedSession = boost::make_shared<WebSocketImpl>(virtualSharedFromThis<PlayerSession>());

			auto responseHeaders = Poseidon::WebSocket::makeHandshakeResponse(requestHeaders);
			if(responseHeaders.statusCode != Poseidon::Http::ST_SWITCHING_PROTOCOLS){
				DEBUG_THROW(Poseidon::Http::Exception, responseHeaders.statusCode);
			}
			Poseidon::Http::Session::send(std::move(responseHeaders), { });

			return std::move(upgradedSession);
		}
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
	}

	return Poseidon::Http::Session::predispatchRequest(requestHeaders, entity);
}

void PlayerSession::onSyncRequest(Poseidon::Http::RequestHeaders requestHeaders, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_GOLD_SCRAMBLE(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted account HTTP request from ", getRemoteInfo());

	auto uri = Poseidon::Http::urlDecode(requestHeaders.uri);
	if((uri.size() < m_path.size() + 1) || (uri.compare(0, m_path.size(), m_path) != 0) || (uri.at(m_path.size()) != '/')){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Inacceptable account HTTP request: uri = ", uri, ", path = ", m_path);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_path.size() + 1);

	if(requestHeaders.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = getHttpServlet(uri);
	if(!servlet){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	Poseidon::JsonObject result;
	try {
		result = (*servlet)(virtualSharedFromThis<PlayerSession>(), std::move(requestHeaders.getParams));
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(std::logic_error &e){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::logic_error thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_BAD_REQUEST);
	} catch(std::exception &e){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_INTERNAL_SERVER_ERROR);
	}
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Account response: uri = ", uri, ", result = ", result.dump());
	Poseidon::OptionalMap headers;
	headers.set(sslit("Access-Control-Allow-Origin"), "*");
	headers.set(sslit("Content-Type"), "application/json; charset=utf-8");
	Poseidon::Http::Session::send(Poseidon::Http::ST_OK, std::move(headers), Poseidon::StreamBuffer(result.dump()));
}

bool PlayerSession::send(boost::uint16_t messageId, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(getUpgradedSession());
	if(!impl){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("WebSocket session comes out of thin air?");
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
		LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
	}
	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(getUpgradedSession());
	if(impl){
		impl->shutdown(Poseidon::WebSocket::ST_NORMAL_CLOSURE);
	}

	Poseidon::Http::Session::shutdownRead();
	Poseidon::Http::Session::shutdownWrite();
}

}
