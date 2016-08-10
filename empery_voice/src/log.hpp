#ifndef EMPERY_VOICE_LOG_HPP_
#define EMPERY_VOICE_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyVoice {

const unsigned long long LOG_CATEGORY = 0x00030800;

}

#define LOG_EMPERY_VOICE(level_, ...)	\
	LOG_MASK(::EmperyVoice::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_VOICE_FATAL(...)        LOG_EMPERY_VOICE(::Poseidon::Logger::LV_FATAL,     __VA_ARGS__)
#define LOG_EMPERY_VOICE_ERROR(...)        LOG_EMPERY_VOICE(::Poseidon::Logger::LV_ERROR,     __VA_ARGS__)
#define LOG_EMPERY_VOICE_WARNING(...)      LOG_EMPERY_VOICE(::Poseidon::Logger::LV_WARNING,   __VA_ARGS__)
#define LOG_EMPERY_VOICE_INFO(...)         LOG_EMPERY_VOICE(::Poseidon::Logger::LV_INFO,      __VA_ARGS__)
#define LOG_EMPERY_VOICE_DEBUG(...)        LOG_EMPERY_VOICE(::Poseidon::Logger::LV_DEBUG,     __VA_ARGS__)
#define LOG_EMPERY_VOICE_TRACE(...)        LOG_EMPERY_VOICE(::Poseidon::Logger::LV_TRACE,     __VA_ARGS__)

#endif
