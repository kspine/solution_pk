#ifndef EMPERY_CENTER_LOG_HPP_
#define EMPERY_CENTER_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyCenter {

const unsigned long long LOG_CATEGORY = 0x00010500;

}

#define LOG_EMPERY_CENTER(level_, ...)	\
	LOG_MASK(::EmperyCenter::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_CENTER_FATAL(...)        LOG_EMPERY_CENTER(::Poseidon::Logger::LV_FATAL,     __VA_ARGS__)
#define LOG_EMPERY_CENTER_ERROR(...)        LOG_EMPERY_CENTER(::Poseidon::Logger::LV_ERROR,     __VA_ARGS__)
#define LOG_EMPERY_CENTER_WARNING(...)      LOG_EMPERY_CENTER(::Poseidon::Logger::LV_WARNING,   __VA_ARGS__)
#define LOG_EMPERY_CENTER_INFO(...)         LOG_EMPERY_CENTER(::Poseidon::Logger::LV_INFO,      __VA_ARGS__)
#define LOG_EMPERY_CENTER_DEBUG(...)        LOG_EMPERY_CENTER(::Poseidon::Logger::LV_DEBUG,     __VA_ARGS__)
#define LOG_EMPERY_CENTER_TRACE(...)        LOG_EMPERY_CENTER(::Poseidon::Logger::LV_TRACE,     __VA_ARGS__)

#endif
