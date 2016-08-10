#include "precompiled.hpp"
#include "voice_file.hpp"
#include "singletons/voice_file_map.hpp"

namespace EmperyVoice {

VoiceFile::VoiceFile(AccountUuid account_uuid, VoiceUuid voice_uuid, Poseidon::StreamBuffer data, std::uint64_t expiry_time)
	: m_account_uuid(account_uuid), m_voice_uuid(voice_uuid)
	, m_data(std::move(data)), m_expiry_time(expiry_time)
{
}
VoiceFile::~VoiceFile(){
}

void VoiceFile::set_data(Poseidon::StreamBuffer data){
	PROFILE_ME;

	m_data = std::move(data);

	// VoiceFileMap::update(virtual_shared_from_this<VoiceFile>(), false);
}

void VoiceFile::set_expiry_time(std::uint64_t expiry_time){
	PROFILE_ME;

	m_expiry_time = std::move(expiry_time);

	VoiceFileMap::update(virtual_shared_from_this<VoiceFile>(), false);
}

}
