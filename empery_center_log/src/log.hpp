#ifndef EMPERY_CENTER_LOG_LOG_HPP_
#define EMPERY_CENTER_LOG_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyCenterLog {

const unsigned long long LOG_CATEGORY = 0x00030800;

}

#define LOG_EMPERY_CENTER_LOG(level_, ...)	\
	LOG_MASK(::EmperyCenterLog::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_CENTER_LOG_FATAL(...)        LOG_EMPERY_CENTER_LOG(::Poseidon::Logger::LV_FATAL,     __VA_ARGS__)
#define LOG_EMPERY_CENTER_LOG_ERROR(...)        LOG_EMPERY_CENTER_LOG(::Poseidon::Logger::LV_ERROR,     __VA_ARGS__)
#define LOG_EMPERY_CENTER_LOG_WARNING(...)      LOG_EMPERY_CENTER_LOG(::Poseidon::Logger::LV_WARNING,   __VA_ARGS__)
#define LOG_EMPERY_CENTER_LOG_INFO(...)         LOG_EMPERY_CENTER_LOG(::Poseidon::Logger::LV_INFO,      __VA_ARGS__)
#define LOG_EMPERY_CENTER_LOG_DEBUG(...)        LOG_EMPERY_CENTER_LOG(::Poseidon::Logger::LV_DEBUG,     __VA_ARGS__)
#define LOG_EMPERY_CENTER_LOG_TRACE(...)        LOG_EMPERY_CENTER_LOG(::Poseidon::Logger::LV_TRACE,     __VA_ARGS__)

#endif
