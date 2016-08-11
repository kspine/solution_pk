#include "../precompiled.hpp"
#include "voice_file_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../voice_file.hpp"

namespace EmperyVoice {

namespace {
	struct FileElement {
		boost::shared_ptr<VoiceFile> file;

		VoiceUuid voice_uuid;
		std::uint64_t expiry_time;
		std::pair<AccountUuid, std::uint64_t> expiry_time_by_account;

		explicit FileElement(boost::shared_ptr<VoiceFile> file_)
			: file(std::move(file_))
			, voice_uuid(file->get_voice_uuid()), expiry_time(file->get_expiry_time())
			, expiry_time_by_account(file->get_account_uuid(), file->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(FileContainer, FileElement,
		UNIQUE_MEMBER_INDEX(voice_uuid)
		MULTI_MEMBER_INDEX(expiry_time)
		MULTI_MEMBER_INDEX(expiry_time_by_account)
	)

	boost::weak_ptr<FileContainer> g_file_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_VOICE_TRACE("Voice file gc timer: now = ", now);

		const auto file_map = g_file_map.lock();
		if(!file_map){
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();

		for(;;){
			const auto it = file_map->begin<1>();
			if(it == file_map->end<1>()){
				break;
			}
			if(utc_now < it->expiry_time){
				break;
			}

			LOG_EMPERY_VOICE_DEBUG("Reclaiming voice file: voice_uuid = ", it->voice_uuid);
			file_map->erase<1>(it);
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto file_map = boost::make_shared<FileContainer>();
		g_file_map = file_map;
		handles.push(file_map);

		const auto gc_interval = get_config<std::uint64_t>("message_expiry_duration", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<VoiceFile> VoiceFileMap::get(VoiceUuid voice_uuid){
	PROFILE_ME;

	const auto file_map = g_file_map.lock();
	if(!file_map){
		LOG_EMPERY_VOICE_WARNING("Voice file map not loaded.");
		return { };
	}

	const auto it = file_map->find<0>(voice_uuid);
	if(it == file_map->end<0>()){
		LOG_EMPERY_VOICE_TRACE("Voice file not found: voice_uuid = ", voice_uuid);
		return { };
	}
	return it->file;
}
void VoiceFileMap::insert(const boost::shared_ptr<VoiceFile> &file){
	PROFILE_ME;

	const auto file_map = g_file_map.lock();
	if(!file_map){
		LOG_EMPERY_VOICE_WARNING("Voice file map not loaded.");
		DEBUG_THROW(Exception, sslit("Voice file map not loaded"));
	}

	const auto account_uuid = file->get_account_uuid();
	const auto voice_uuid = file->get_voice_uuid();

	LOG_EMPERY_VOICE_DEBUG("Inserting voice: voice_uuid = ", voice_uuid);
	if(!file_map->insert(FileElement(file)).second){
		LOG_EMPERY_VOICE_WARNING("Voice file already exists: voice_uuid = ", voice_uuid);
		DEBUG_THROW(Exception, sslit("Voice file already exists"));
	}

	auto range = std::make_pair(file_map->lower_bound<2>(std::make_pair(account_uuid, 0)),
	                            file_map->upper_bound<2>(std::make_pair(account_uuid, UINT64_MAX)));
	const auto count_max = get_config<std::size_t>("max_messages_per_account", 20);
	const auto count = static_cast<std::size_t>(std::distance(range.first, range.second));
	for(std::size_t i = count_max; i < count; ++i){
		range.first = file_map->erase<2>(range.first);
	}
}
void VoiceFileMap::update(const boost::shared_ptr<VoiceFile> &file, bool throws_if_not_exists){
	PROFILE_ME;

	const auto file_map = g_file_map.lock();
	if(!file_map){
		LOG_EMPERY_VOICE_WARNING("Voice file map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Voice file map not loaded"));
		}
		return;
	}

	const auto voice_uuid = file->get_voice_uuid();

	const auto it = file_map->find<0>(voice_uuid);
	if(it == file_map->end<0>()){
		LOG_EMPERY_VOICE_WARNING("Voice file not found: voice_uuid = ", voice_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Voice file map not loaded"));
		}
		return;
	}
	if(it->file != file){
		LOG_EMPERY_VOICE_DEBUG("Voice file expired: voice_uuid = ", voice_uuid);
		return;
	}

	LOG_EMPERY_VOICE_DEBUG("Updating voice: voice_uuid = ", voice_uuid);
	file_map->replace<0>(it, FileElement(file));
}

}
