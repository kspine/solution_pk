#ifndef EMPERY_GOLD_SCRAMBLE_LOG_HPP_
#define EMPERY_GOLD_SCRAMBLE_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyGoldScramble {

const unsigned long long LOG_CATEGORY = 0x00017100;

}

#define LOG_EMPERY_GOLD_SCRAMBLE(level_, ...)	\
	LOG_MASK(::EmperyGoldScramble::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_GOLD_SCRAMBLE_FATAL(...)     LOG_EMPERY_GOLD_SCRAMBLE(::Poseidon::Logger::LV_FATAL,      __VA_ARGS__)
#define LOG_EMPERY_GOLD_SCRAMBLE_ERROR(...)     LOG_EMPERY_GOLD_SCRAMBLE(::Poseidon::Logger::LV_ERROR,      __VA_ARGS__)
#define LOG_EMPERY_GOLD_SCRAMBLE_WARNING(...)   LOG_EMPERY_GOLD_SCRAMBLE(::Poseidon::Logger::LV_WARNING,    __VA_ARGS__)
#define LOG_EMPERY_GOLD_SCRAMBLE_INFO(...)      LOG_EMPERY_GOLD_SCRAMBLE(::Poseidon::Logger::LV_INFO,       __VA_ARGS__)
#define LOG_EMPERY_GOLD_SCRAMBLE_DEBUG(...)     LOG_EMPERY_GOLD_SCRAMBLE(::Poseidon::Logger::LV_DEBUG,      __VA_ARGS__)
#define LOG_EMPERY_GOLD_SCRAMBLE_TRACE(...)     LOG_EMPERY_GOLD_SCRAMBLE(::Poseidon::Logger::LV_TRACE,      __VA_ARGS__)

#endif
