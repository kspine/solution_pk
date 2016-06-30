#ifndef EMPERY_DUNGEON_LOG_HPP_
#define EMPERY_DUNGEON_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyDungeon {

const unsigned long long LOG_CATEGORY = 0x00010900;

}

#define LOG_EMPERY_DUNGEON(level_, ...)	\
	LOG_MASK(::EmperyDungeon::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_DUNGEON_FATAL(...)        LOG_EMPERY_DUNGEON(::Poseidon::Logger::LV_FATAL,     __VA_ARGS__)
#define LOG_EMPERY_DUNGEON_ERROR(...)        LOG_EMPERY_DUNGEON(::Poseidon::Logger::LV_ERROR,     __VA_ARGS__)
#define LOG_EMPERY_DUNGEON_WARNING(...)      LOG_EMPERY_DUNGEON(::Poseidon::Logger::LV_WARNING,   __VA_ARGS__)
#define LOG_EMPERY_DUNGEON_INFO(...)         LOG_EMPERY_DUNGEON(::Poseidon::Logger::LV_INFO,      __VA_ARGS__)
#define LOG_EMPERY_DUNGEON_DEBUG(...)        LOG_EMPERY_DUNGEON(::Poseidon::Logger::LV_DEBUG,     __VA_ARGS__)
#define LOG_EMPERY_DUNGEON_TRACE(...)        LOG_EMPERY_DUNGEON(::Poseidon::Logger::LV_TRACE,     __VA_ARGS__)

#endif
