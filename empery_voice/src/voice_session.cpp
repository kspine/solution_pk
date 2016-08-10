#include "precompiled.hpp"
#include "voice_session.hpp"
#include "mmain.hpp"
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/json.hpp>
#include "voice_file.hpp"
#include "singletons/voice_file_map.hpp"
#include "singletons/controller_client.hpp"
#include "../../empery_center/src/msg/kill.hpp"
#include "../../empery_center/src/msg/st_account.hpp"

namespace EmperyVoice {

namespace Msg = ::EmperyCenter::Msg;

VoiceSession::VoiceSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket), get_config<std::uint64_t>("max_bytes_per_message", 0))
	, m_prefix(std::move(prefix))
{
}
VoiceSession::~VoiceSession(){
}

void VoiceSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity){
	PROFILE_ME;
	LOG_EMPERY_VOICE_INFO("Accepted voice HTTP request from ", get_remote_info());

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_VOICE_WARNING("Inacceptable voice HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(!uri.empty()){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
	}

	const auto controller = ControllerClient::require();
	const auto &params = request_headers.get_params;

	if(request_headers.verb == Poseidon::Http::V_PUT){
		const auto account_uuid = AccountUuid(params.at("accountUuid"));
		Msg::ST_AccountCheckIp treq;
		treq.account_uuid = account_uuid.str();
		treq.ip_expected  = get_remote_info().ip.get();
		auto tresult = controller->send_and_wait(treq);
		if(tresult.first != Msg::ST_OK){
			LOG_EMPERY_VOICE_DEBUG("Controller server returned an error: code = ", tresult.first, ", msg = ", tresult.second);
			DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
		}

		const auto expiry_duration = get_config<std::uint64_t>("message_expiry_duration", 300000);

		const auto utc_now = Poseidon::get_utc_time();
		const auto voice_uuid = VoiceUuid(Poseidon::Uuid::random());

		const auto voice_file = boost::make_shared<VoiceFile>(account_uuid, voice_uuid, std::move(entity), utc_now + expiry_duration);
		VoiceFileMap::insert(voice_file);

		Poseidon::JsonObject response;
		response[sslit("voiceUuid")]      = voice_uuid.str();
		response[sslit("expiryDuration")] = expiry_duration;

		Poseidon::OptionalMap headers;
		headers.set(sslit("Content-Type"), "application/json");
		send(Poseidon::Http::ST_OK, std::move(headers), Poseidon::StreamBuffer(response.dump()));
	} else if(request_headers.verb == Poseidon::Http::V_GET){
		const auto account_uuid = AccountUuid(params.at("accountUuid"));
		Msg::ST_AccountCheckIp treq;
		treq.account_uuid = account_uuid.str();
		treq.ip_expected  = get_remote_info().ip.get();
		auto tresult = controller->send_and_wait(treq);
		if(tresult.first != Msg::ST_OK){
			LOG_EMPERY_VOICE_DEBUG("Controller server returned an error: code = ", tresult.first, ", msg = ", tresult.second);
			DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_FORBIDDEN);
		}

		const auto voice_uuid = VoiceUuid(params.at("voiceUuid"));
		const auto voice_file = VoiceFileMap::get(voice_uuid);
		if(!voice_file){
			DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
		}
		Poseidon::OptionalMap headers;
		headers.set(sslit("Content-Type"), "application/octet-stream");
		send(Poseidon::Http::ST_OK, std::move(headers), voice_file->get_data());
	} else {
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}
}

}
