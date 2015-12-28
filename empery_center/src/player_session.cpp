#include "precompiled.hpp"
#include "player_session.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/cbpp/control_message.hpp>
#include <poseidon/cbpp/control_codes.hpp>
#include <poseidon/cbpp/status_codes.hpp>
#include <poseidon/cbpp/exception.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/websocket/session.hpp>
#include <poseidon/websocket/status_codes.hpp>
#include <poseidon/websocket/exception.hpp>
#include <poseidon/websocket/handshake.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "singletons/player_session_map.hpp"
#include "msg/kill.hpp"
#include "msg/sc_packed.hpp"
#include "singletons/world_map.hpp"

namespace EmperyCenter {

using ServletCallback = PlayerSession::ServletCallback;

namespace {
	boost::container::flat_map<unsigned, boost::weak_ptr<const ServletCallback>> g_servlet_map;
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
			if(opcode != Poseidon::WebSocket::OP_DATA_BIN){
				LOG_EMPERY_CENTER_WARNING("Invalid message type: opcode = ", opcode);
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			auto read = Poseidon::StreamBuffer::ReadIterator(payload);
			boost::uint64_t message_id64;
			if(!Poseidon::vuint50_from_binary(message_id64, read, payload.size())){
				LOG_EMPERY_CENTER_WARNING("Packet too small");
				DEBUG_THROW(Poseidon::WebSocket::Exception, Poseidon::WebSocket::ST_INACCEPTABLE);
			}
			message_id = message_id64 & 0xFFFF;
			if(message_id != message_id64){
				LOG_EMPERY_CENTER_WARNING("Message id too large: message_id64 = ", message_id64);
				DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
			}

			if(message_id == Poseidon::Cbpp::ControlMessage::ID){
				const auto req = Poseidon::Cbpp::ControlMessage(std::move(payload));
				LOG_EMPERY_CENTER_TRACE("Received ping from ", parent->get_remote_info(), ": req = ", req);
				result.first = Poseidon::Cbpp::ST_PONG;
				result.second = std::move(req.string_param);
			} else {
				const auto servlet = PlayerSession::get_servlet(message_id);
				if(!servlet){
					LOG_EMPERY_CENTER_WARNING("No servlet found: message_id = ", message_id);
					DEBUG_THROW(Poseidon::Cbpp::Exception, Poseidon::Cbpp::ST_NOT_FOUND);
				}
				result = (*servlet)(parent, std::move(payload));
			}
		} catch(Poseidon::Cbpp::Exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"Poseidon::Cbpp::Exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = e.status_code();
			result.second = e.what();
		} catch(std::exception &e){
			LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
				"std::exception thrown: message_id = ", message_id, ", what = ", e.what());
			result.first = Poseidon::Cbpp::ST_INTERNAL_ERROR;
			result.second = e.what();
		}
		if(result.first != 0){
			LOG_EMPERY_CENTER_DEBUG("Sending response: message_id = ", message_id, ", code = ", result.first, ", message = ", result.second);
		}
		if(result.first < 0){
			parent->shutdown(result.first, result.second.c_str());
		} else {
			parent->send_control(message_id, result.first, std::move(result.second));
		}
	}
};

class PlayerSession::QueueImpl : public Poseidon::JobBase {
private:
	const boost::weak_ptr<PlayerSession> m_session;

public:
	explicit QueueImpl(const boost::shared_ptr<PlayerSession> &session)
		: m_session(session)
	{
	}

public:
	boost::weak_ptr<const void> get_category() const final {
		return m_session;
	}
	void perform() final {
		PROFILE_ME;

		const auto session = m_session.lock();
		if(!session){
			return;
		}

		auto &queue = session->m_send_queue;
		if(queue.empty()){
			return;
		}
		const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(session->get_upgraded_session());
		if(!impl){
			LOG_EMPERY_CENTER_WARNING("WebSocket session comes out of thin air?");
			return;
		}

		LOG_EMPERY_CENTER_DEBUG("Pumping queued messages: queue_size = ", queue.size());

		try {
			Poseidon::StreamBuffer contents;
			auto wit = Poseidon::StreamBuffer::WriteIterator(contents);
/*
			#define MESSAGE_NAME    SC_PackedResponse
			#define MESSAGE_ID      69
			#define MESSAGE_FIELDS  \
				FIELD_ARRAY         (messages,  \
					FIELD_VUINT         (message_id)    \
					FIELD_STRING        (payload)   \
				)
			#include <poseidon/cbpp/message_generator.hpp>
*/
			Poseidon::vuint50_to_binary(Msg::SC_PackedResponse::ID, wit);
			Poseidon::vuint50_to_binary(queue.size(), wit);
			while(!queue.empty()){
				auto message = std::move(queue.front());
				queue.pop_front();

				Poseidon::vuint50_to_binary(message.first, wit);
				Poseidon::vuint50_to_binary(message.second.size(), wit);
				contents.splice(message.second);
			}

			impl->send(std::move(contents), true);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
};

boost::shared_ptr<const ServletCallback> PlayerSession::create_servlet(boost::uint16_t message_id, ServletCallback callback){
	PROFILE_ME;

	auto &weak = g_servlet_map[message_id];
	if(!weak.expired()){
		LOG_EMPERY_CENTER_ERROR("Duplicate player servlet: message_id = ", message_id);
		DEBUG_THROW(Exception, sslit("Duplicate player servlet"));
	}

	auto servlet = boost::make_shared<ServletCallback>(std::move(callback));
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const ServletCallback> PlayerSession::get_servlet(boost::uint16_t message_id){
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

PlayerSession::PlayerSession(Poseidon::UniqueFile socket, std::string path)
	: Poseidon::Http::Session(std::move(socket))
	, m_path(std::move(path))
	, m_view(INT64_MIN, INT64_MIN, 0, 0)
{
}
PlayerSession::~PlayerSession(){
}

void PlayerSession::on_close(int err_code) noexcept {
	PROFILE_ME;
	LOG_EMPERY_CENTER_INFO("Socket close: err_code = ", err_code, ", description = ", Poseidon::get_error_desc(err_code));

	PlayerSessionMap::async_begin_gc();

	Poseidon::Http::Session::on_close(err_code);
}

boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> PlayerSession::predispatch_request(
	Poseidon::Http::RequestHeaders &request_headers, Poseidon::StreamBuffer &entity)
{
	PROFILE_ME;

	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_IMPLEMENTED);
	}
	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if(uri == m_path){
		if(::strcasecmp(request_headers.headers.get("Upgrade").c_str(), "websocket") == 0){
			auto upgraded_session = boost::make_shared<WebSocketImpl>(virtual_shared_from_this<PlayerSession>());

			auto response_headers = Poseidon::WebSocket::make_handshake_response(request_headers);
			if(response_headers.status_code != Poseidon::Http::ST_SWITCHING_PROTOCOLS){
				DEBUG_THROW(Poseidon::Http::Exception, response_headers.status_code);
			}
			Poseidon::Http::Session::send(std::move(response_headers), { });

			return std::move(upgraded_session);
		}
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
	}

	return Poseidon::Http::Session::predispatch_request(request_headers, entity);
}

void PlayerSession::on_sync_request(Poseidon::Http::RequestHeaders /* request_headers */, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;

	DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
}

namespace {
}

bool PlayerSession::send(boost::uint16_t message_id, Poseidon::StreamBuffer payload){
	PROFILE_ME;
/*
	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(get_upgraded_session());
	if(!impl){
		LOG_EMPERY_CENTER_WARNING("WebSocket session comes out of thin air?");
		DEBUG_THROW(Exception, sslit("WebSocket session comes out of thin air"));
	}

	Poseidon::StreamBuffer whole;
	auto wit = Poseidon::StreamBuffer::WriteIterator(whole);
	Poseidon::vuint50_to_binary(message_id, wit);
	whole.splice(payload);
	return impl->send(std::move(whole), true);
*/
	if(m_send_queue.empty()){
		Poseidon::JobDispatcher::enqueue(
			boost::make_shared<QueueImpl>(virtual_shared_from_this<PlayerSession>()),
			{ }, { });
	}
	m_send_queue.emplace_back(message_id, std::move(payload));
	return true;
}
bool PlayerSession::send_control(boost::uint16_t message_id, int status_code, std::string reason){
	PROFILE_ME;

	return send(Poseidon::Cbpp::ControlMessage(message_id, static_cast<int>(status_code), std::move(reason)));
}

void PlayerSession::shutdown(const char *message) noexcept {
	PROFILE_ME;

	shutdown(Poseidon::Cbpp::ST_INTERNAL_ERROR, message);
}
void PlayerSession::shutdown(int reason, const char *message) noexcept {
	PROFILE_ME;

	if(!message){
		message = "";
	}
	try {
		send_control(Poseidon::Cbpp::ControlMessage::ID, reason, message);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}
	const auto impl = boost::dynamic_pointer_cast<WebSocketImpl>(get_upgraded_session());
	if(impl){
		impl->shutdown(Poseidon::WebSocket::ST_NORMAL_CLOSURE);
	}

	Poseidon::Http::Session::shutdown_read();
	Poseidon::Http::Session::shutdown_write();
}

void PlayerSession::set_view(Rectangle view){
	PROFILE_ME;

	m_view = view;

	WorldMap::update_player_view(virtual_shared_from_this<PlayerSession>());
}

}
