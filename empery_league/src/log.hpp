#ifndef EMPERY_LEAGUE_LOG_HPP_
#define EMPERY_LEAGUE_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyLeague {

const unsigned long long LOG_CATEGORY = 0x00020500;

}

#define LOG_EMPERY_LEAGUE(level_, ...)	\
	LOG_MASK(::EmperyLeague::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_LEAGUE_FATAL(...)        LOG_EMPERY_LEAGUE(::Poseidon::Logger::LV_FATAL,     __VA_ARGS__)
#define LOG_EMPERY_LEAGUE_ERROR(...)        LOG_EMPERY_LEAGUE(::Poseidon::Logger::LV_ERROR,     __VA_ARGS__)
#define LOG_EMPERY_LEAGUE_WARNING(...)      LOG_EMPERY_LEAGUE(::Poseidon::Logger::LV_WARNING,   __VA_ARGS__)
#define LOG_EMPERY_LEAGUE_INFO(...)         LOG_EMPERY_LEAGUE(::Poseidon::Logger::LV_INFO,      __VA_ARGS__)
#define LOG_EMPERY_LEAGUE_DEBUG(...)        LOG_EMPERY_LEAGUE(::Poseidon::Logger::LV_DEBUG,     __VA_ARGS__)
#define LOG_EMPERY_LEAGUE_TRACE(...)        LOG_EMPERY_LEAGUE(::Poseidon::Logger::LV_TRACE,     __VA_ARGS__)

#endif
