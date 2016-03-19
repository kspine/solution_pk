#ifndef EMPERY_ACCOUNT_LOG_HPP_
#define EMPERY_ACCOUNT_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyAccount {

const unsigned long long LOG_CATEGORY = 0x00035200;

}

#define LOG_EMPERY_ACCOUNT(level_, ...)	\
	LOG_MASK(::EmperyAccount::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_ACCOUNT_FATAL(...)     LOG_EMPERY_ACCOUNT(::Poseidon::Logger::LV_FATAL,      __VA_ARGS__)
#define LOG_EMPERY_ACCOUNT_ERROR(...)     LOG_EMPERY_ACCOUNT(::Poseidon::Logger::LV_ERROR,      __VA_ARGS__)
#define LOG_EMPERY_ACCOUNT_WARNING(...)   LOG_EMPERY_ACCOUNT(::Poseidon::Logger::LV_WARNING,    __VA_ARGS__)
#define LOG_EMPERY_ACCOUNT_INFO(...)      LOG_EMPERY_ACCOUNT(::Poseidon::Logger::LV_INFO,       __VA_ARGS__)
#define LOG_EMPERY_ACCOUNT_DEBUG(...)     LOG_EMPERY_ACCOUNT(::Poseidon::Logger::LV_DEBUG,      __VA_ARGS__)
#define LOG_EMPERY_ACCOUNT_TRACE(...)     LOG_EMPERY_ACCOUNT(::Poseidon::Logger::LV_TRACE,      __VA_ARGS__)

#endif
