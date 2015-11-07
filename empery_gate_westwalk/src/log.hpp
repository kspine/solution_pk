#ifndef EMPERY_GATE_WESTWALK_LOG_HPP_
#define EMPERY_GATE_WESTWALK_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyGateWestwalk {

const unsigned long long LOG_CATEGORY = 0x00017000;

}

#define LOG_EMPERY_GATE_WESTWALK(level_, ...)	\
	LOG_MASK(::EmperyGateWestwalk::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_GATE_WESTWALK_FATAL(...)         LOG_EMPERY_GATE_WESTWALK(::Poseidon::Logger::LV_FATAL,      __VA_ARGS__)
#define LOG_EMPERY_GATE_WESTWALK_ERROR(...)         LOG_EMPERY_GATE_WESTWALK(::Poseidon::Logger::LV_ERROR,      __VA_ARGS__)
#define LOG_EMPERY_GATE_WESTWALK_WARNING(...)       LOG_EMPERY_GATE_WESTWALK(::Poseidon::Logger::LV_WARNING,    __VA_ARGS__)
#define LOG_EMPERY_GATE_WESTWALK_INFO(...)          LOG_EMPERY_GATE_WESTWALK(::Poseidon::Logger::LV_INFO,       __VA_ARGS__)
#define LOG_EMPERY_GATE_WESTWALK_DEBUG(...)         LOG_EMPERY_GATE_WESTWALK(::Poseidon::Logger::LV_DEBUG,      __VA_ARGS__)
#define LOG_EMPERY_GATE_WESTWALK_TRACE(...)         LOG_EMPERY_GATE_WESTWALK(::Poseidon::Logger::LV_TRACE,      __VA_ARGS__)

#endif
