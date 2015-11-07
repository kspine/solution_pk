#ifndef EMPERY_PROMOTION_LOG_LOG_HPP_
#define EMPERY_PROMOTION_LOG_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyPromotionLog {

const unsigned long long LOG_CATEGORY = 0x00035200;

}

#define LOG_EMPERY_PROMOTION_LOG(level_, ...)	\
	LOG_MASK(::EmperyPromotionLog::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_PROMOTION_LOG_FATAL(...)     LOG_EMPERY_PROMOTION_LOG(::Poseidon::Logger::LV_FATAL,      __VA_ARGS__)
#define LOG_EMPERY_PROMOTION_LOG_ERROR(...)     LOG_EMPERY_PROMOTION_LOG(::Poseidon::Logger::LV_ERROR,      __VA_ARGS__)
#define LOG_EMPERY_PROMOTION_LOG_WARNING(...)   LOG_EMPERY_PROMOTION_LOG(::Poseidon::Logger::LV_WARNING,    __VA_ARGS__)
#define LOG_EMPERY_PROMOTION_LOG_INFO(...)      LOG_EMPERY_PROMOTION_LOG(::Poseidon::Logger::LV_INFO,       __VA_ARGS__)
#define LOG_EMPERY_PROMOTION_LOG_DEBUG(...)     LOG_EMPERY_PROMOTION_LOG(::Poseidon::Logger::LV_DEBUG,      __VA_ARGS__)
#define LOG_EMPERY_PROMOTION_LOG_TRACE(...)     LOG_EMPERY_PROMOTION_LOG(::Poseidon::Logger::LV_TRACE,      __VA_ARGS__)

#endif
