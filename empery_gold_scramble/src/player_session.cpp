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
	std::map<unsigned, boost::weak_ptr<const ServletCallback>>        g_servlet_map;
	std::map<std::string, boost::weak_ptr<const HttpServletCallback>> g_http_servlet_map;
}

class PlayerSession::WebSocketImpl : public Poseidon::WebSocket::Session {
public:
	explicit WebSocketImpl(const boost::shared_ptr<PlayerSession> &parent)
		: Poseidon::WebSocket::Session(parent)
	{
	}

protected:
	void on_sync_data_message(Poseidon::WebSocket::OpCode opcode, Poseidon::StreamBuffer payload) override {
		PROFILE_ME;

		const auto parent = boost::dynamic_pointer_cast<PlayerSession>(get_parent());
		if(!parent){
			force_shutdown();
			return;
		}

		unsigned message_id = UINT_MAX;
		std::pair<long, std::string> result;
		try {
			if(opcode != Poseidon::WebSocket::OP_DATA_BINARY){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Invalid message type: opcode = ", opcode);
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			auto read = Poseidon::StreamBuffer::ReadIterator(payload);
			std::uint64_t message_id64;
			if(!Poseidon::vuint64_from_binary(message_id64, read, payload.size())){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Packet too small");
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			message_id = message_id64 & 0xFFFF;
			if(message_id != message_id64){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Message id too large: message_id64 = ", message_id64);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
			}

			if(message_id == Poseidon::Cbpp::ControlMessage::ID){
				const auto req = Poseidon::Cbpp::ControlMessage(std::move(payload));
				LOG_EMPERY_GOLD_SCRAMBLE_TRACE("Received ping from ", parent->get_remote_info(), ": req = ", req);
				result.first = Poseidon::Cbpp::ST_PONG;
				result.second = std::move(req.string_param);
			} else {
				const auto servlet = PlayerSession::get_servlet(message_id);
				if(!servlet){
					LOG_EMPERY_GOLD_SCRAMBLE_WARNING("No servlet found: message_id = ", message_id);
					DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
				}
				result = (*servlet)(parent, std::move(payload));
			}
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = e.status_code();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = Msg::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		if(message_id != Poseidon::Cbpp::ControlMessage::ID){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Sending response: message_id = ", message_id,
				", code = ", result.first, ", message = ", result.second);
		}
		if(result.first < 0){
			parent->shutdown(result.first, result.second.c_str());
		} else {
			parent->send_control(message_id, result.first, std::move(result.second));
		}
	}
};

boost::shared_ptr<const ServletCallback> PlayerSession::create_servlet(std::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	auto &weak_servlet = g_servlet_map[message_id];
	if(!weak_servlet.expired()){
		LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Duplicate player servlet: message_id = ", message_id);
		DEBUG_THROW(Exception, sslit("Duplicate player servlet"));
	}
	weak_servlet = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> PlayerSession::get_servlet(std::uint16_t message_id){
	PROFILE_ME;

	const auto it = g_servlet_map.find(message_id);
	if(it == g_servlet_map.end()){
		return { };
	}
	auto servlet = it->second.lock();
	if(!servlet){
		g_servlet_map.erase(it);
		return { };
	}
	return servlet;
}

boost::shared_ptr<const HttpServletCallback> PlayerSession::create_http_servlet(const std::string &uri, HttpServletCallback callback){
	PROFILE_ME;

	auto servlet = boost::make_shared<HttpServletCallback>(std::move(callback));
	if(!g_http_servlet_map.insert(std::make_pair(uri, servlet)).second){
		LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Duplicate player HTTP servlet: uri = ", uri);
		DEBUG_THROW(Exception, sslit("Duplicate player HTTP servlet"));
	}
	return std::move(servlet);
}
boost::shared_ptr<const HttpServletCallback> PlayerSession::get_http_servlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_http_servlet_map.find(uri);
	if(it == g_http_servlet_map.end()){
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

void PlayerSession::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Socket close: err_code = ", err_code, ", description = ", Poseidon::get_error_desc(err_code));

	try {
		Poseidon::enqueue_async_job(virtual_weak_from_this<PlayerSession>(),
			boost::bind(&PlayerSessionMap::remove, virtual_weak_from_this<PlayerSession>()));
	} catch(std::exception &e){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
	}

	Poseidon::Http::Session::on_close(err_code);
}

boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> PlayerSession::on_low_level_request_end(
	std::uint64_t content_length, bool is_chunked, Poseidon::OptionalMap headers)
{
	PROFILE_ME;

	auto upgraded_session = Poseidon::Http::Session::on_low_level_request_end(content_length, is_chunked, std::move(headers));
	if(upgraded_session){
		return std::move(upgraded_session);
	}

	const auto &request_headers = get_low_level_request_headers();
	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_IMPLEMENTED);
	}
	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if(uri == m_path){
		if(::strcasecmp(request_headers.headers.get("Upgrade").c_str(), "websocket") == 0){
			upgraded_session = boost::make_shared<WebSocketImpl>(virtual_shared_from_this<PlayerSession>());
			auto response_headers = Poseidon::WebSocket::make_handshake_response(request_headers);
			if(response_headers.status_code != Poseidon::Http::ST_SWITCHING_PROTOCOLS){
				DEBUG_THROW(Poseidon::Http::Exception, response_headers.status_code);
			}
			Poseidon::Http::Session::send(std::move(response_headers), { });
			return std::move(upgraded_session);
		}
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
	}
	return { };
}

void PlayerSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_GOLD_SCRAMBLE(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted account HTTP request from ", get_remote_info());

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_path.size() + 1) || (uri.compare(0, m_path.size(), m_path) != 0) || (uri.at(m_path.size()) != '/')){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Inacceptable account HTTP request: uri = ", uri, ", path = ", m_path);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_path.size() + 1);

	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	const auto servlet = get_http_servlet(uri);
	if(!servlet){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("No servlet available: uri = ", uri);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}

	Poseidon::JsonObject result;
	try {
		result = (*servlet)(virtual_shared_from_this<PlayerSession>(), std::move(request_headers.get_params));
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

bool PlayerSession::send(std::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;

	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(get_upgraded_session());
	if(!impl){
		LOG_EMPERY_GOLD_SCRAMBLE_WARNING("WebSocket session comes out of thin air?");
		DEBUG_THROW(Exception, sslit("WebSocket session comes out of thin air"));
	}

	Poseidon::StreamBuffer whole;
	auto wit = Poseidon::StreamBuffer::WriteIterator(whole);
	Poseidon::vuint64_to_binary(message_id, wit);
	whole.splice(payload);
	return impl->send(Poseidon::WebSocket::OP_DATA_BINARY, std::move(whole));
}

void PlayerSession::shutdown(int reason, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		send_control(Poseidon::Cbpp::ControlMessage::ID, reason, message);
	} catch(std::exception &e){
		LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
	}
	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(get_upgraded_session());
	if(impl){
		impl->shutdown(Poseidon::WebSocket::ST_NORMAL_CLOSURE);
	}

	Poseidon::Http::Session::shutdown_read();
	Poseidon::Http::Session::shutdown_write();
}

}
