#ifndef EMPERY_MAP_LOG_HPP_
#define EMPERY_MAP_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyMap {

const unsigned long long LOG_CATEGORY = 0x00010600;

}

#define LOG_EMPERY_MAP(level_, ...)	\
	LOG_MASK(::EmperyMap::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_MAP_FATAL(...)		LOG_EMPERY_MAP(::Poseidon::Logger::LV_FATAL,		__VA_ARGS__)
#define LOG_EMPERY_MAP_ERROR(...)		LOG_EMPERY_MAP(::Poseidon::Logger::LV_ERROR,		__VA_ARGS__)
#define LOG_EMPERY_MAP_WARNING(...)		LOG_EMPERY_MAP(::Poseidon::Logger::LV_WARNING,	__VA_ARGS__)
#define LOG_EMPERY_MAP_INFO(...)		LOG_EMPERY_MAP(::Poseidon::Logger::LV_INFO,		__VA_ARGS__)
#define LOG_EMPERY_MAP_DEBUG(...)		LOG_EMPERY_MAP(::Poseidon::Logger::LV_DEBUG,		__VA_ARGS__)
#define LOG_EMPERY_MAP_TRACE(...)		LOG_EMPERY_MAP(::Poseidon::Logger::LV_TRACE,		__VA_ARGS__)

#endif
