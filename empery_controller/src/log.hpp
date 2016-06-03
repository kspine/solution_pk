#ifndef EMPERY_CONTROLLER_LOG_HPP_
#define EMPERY_CONTROLLER_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyController {

const unsigned long long LOG_CATEGORY = 0x00010200;

}

#define LOG_EMPERY_CONTROLLER(level_, ...)	\
	LOG_MASK(::EmperyController::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_CONTROLLER_FATAL(...)        LOG_EMPERY_CONTROLLER(::Poseidon::Logger::LV_FATAL,     __VA_ARGS__)
#define LOG_EMPERY_CONTROLLER_ERROR(...)        LOG_EMPERY_CONTROLLER(::Poseidon::Logger::LV_ERROR,     __VA_ARGS__)
#define LOG_EMPERY_CONTROLLER_WARNING(...)      LOG_EMPERY_CONTROLLER(::Poseidon::Logger::LV_WARNING,   __VA_ARGS__)
#define LOG_EMPERY_CONTROLLER_INFO(...)         LOG_EMPERY_CONTROLLER(::Poseidon::Logger::LV_INFO,      __VA_ARGS__)
#define LOG_EMPERY_CONTROLLER_DEBUG(...)        LOG_EMPERY_CONTROLLER(::Poseidon::Logger::LV_DEBUG,     __VA_ARGS__)
#define LOG_EMPERY_CONTROLLER_TRACE(...)        LOG_EMPERY_CONTROLLER(::Poseidon::Logger::LV_TRACE,     __VA_ARGS__)

#endif
