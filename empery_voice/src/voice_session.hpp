#ifndef EMPERY_VOICE_VOICE_SESSION_HPP_
#define EMPERY_VOICE_VOICE_SESSION_HPP_

#include <poseidon/http/session.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyVoice {

class VoiceSession : public Poseidon::Http::Session {
private:
	const std::string m_prefix;

public:
	VoiceSession(Poseidon::UniqueFile socket, std::string prefix);
	~VoiceSession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
