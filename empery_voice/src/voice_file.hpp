#ifndef EMPERY_VOICE_VOICE_FILE_HPP_
#define EMPERY_VOICE_VOICE_FILE_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/stream_buffer.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyVoice {

class VoiceFile : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const AccountUuid m_account_uuid;
	const VoiceUuid m_voice_uuid;

	Poseidon::StreamBuffer m_data;
	std::uint64_t m_expiry_time;

public:
	VoiceFile(AccountUuid account_uuid, VoiceUuid voice_uuid, Poseidon::StreamBuffer data, std::uint64_t expiry_time);
	~VoiceFile();

public:
	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}
	VoiceUuid get_voice_uuid() const {
		return m_voice_uuid;
	}

	const Poseidon::StreamBuffer &get_data() const {
		return m_data;
	}
	void set_data(Poseidon::StreamBuffer data);

	std::uint64_t get_expiry_time() const {
		return m_expiry_time;
	}
	void set_expiry_time(std::uint64_t expiry_time);
};

}

#endif
