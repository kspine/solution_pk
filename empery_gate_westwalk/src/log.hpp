#ifndef TEXAS_GATE_WESTWALK_LOG_HPP_
#define TEXAS_GATE_WESTWALK_LOG_HPP_

#include <poseidon/log.hpp>

namespace TexasGateWestwalk {

const unsigned long long LOG_CATEGORY = 0x00010200;

}

#define LOG_TEXAS_GATE_WESTWALK(level_, ...)	\
	LOG_MASK(::TexasGateWestwalk::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_TEXAS_GATE_WESTWALK_FATAL(...)			LOG_TEXAS_GATE_WESTWALK(::Poseidon::Logger::LV_FATAL,		__VA_ARGS__)
#define LOG_TEXAS_GATE_WESTWALK_ERROR(...)			LOG_TEXAS_GATE_WESTWALK(::Poseidon::Logger::LV_ERROR,		__VA_ARGS__)
#define LOG_TEXAS_GATE_WESTWALK_WARNING(...)		LOG_TEXAS_GATE_WESTWALK(::Poseidon::Logger::LV_WARNING,		__VA_ARGS__)
#define LOG_TEXAS_GATE_WESTWALK_INFO(...)			LOG_TEXAS_GATE_WESTWALK(::Poseidon::Logger::LV_INFO,		__VA_ARGS__)
#define LOG_TEXAS_GATE_WESTWALK_DEBUG(...)			LOG_TEXAS_GATE_WESTWALK(::Poseidon::Logger::LV_DEBUG,		__VA_ARGS__)
#define LOG_TEXAS_GATE_WESTWALK_TRACE(...)			LOG_TEXAS_GATE_WESTWALK(::Poseidon::Logger::LV_TRACE,		__VA_ARGS__)

#endif
