#ifndef EMPERY_VOICE_SINGLETONS_VOICE_FILE_MAP_HPP_
#define EMPERY_VOICE_SINGLETONS_VOICE_FILE_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyVoice {

class VoiceFile;

struct VoiceFileMap {
	static boost::shared_ptr<VoiceFile> get(VoiceUuid voice_uuid);
	static void insert(const boost::shared_ptr<VoiceFile> &file);
	static void update(const boost::shared_ptr<VoiceFile> &file, bool throws_if_not_exists = true);

private:
	VoiceFileMap() = delete;
};

}

#endif
