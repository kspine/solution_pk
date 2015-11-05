#ifndef EMPERY_CLUSTER_LOG_HPP_
#define EMPERY_CLUSTER_LOG_HPP_

#include <poseidon/log.hpp>

namespace EmperyCluster {

const unsigned long long LOG_CATEGORY = 0x00010600;

}

#define LOG_EMPERY_CLUSTER(level_, ...)	\
	LOG_MASK(::EmperyCluster::LOG_CATEGORY | (level_), __VA_ARGS__)

#define LOG_EMPERY_CLUSTER_FATAL(...)       LOG_EMPERY_CLUSTER(::Poseidon::Logger::LV_FATAL,    __VA_ARGS__)
#define LOG_EMPERY_CLUSTER_ERROR(...)       LOG_EMPERY_CLUSTER(::Poseidon::Logger::LV_ERROR,    __VA_ARGS__)
#define LOG_EMPERY_CLUSTER_WARNING(...)     LOG_EMPERY_CLUSTER(::Poseidon::Logger::LV_WARNING,  __VA_ARGS__)
#define LOG_EMPERY_CLUSTER_INFO(...)        LOG_EMPERY_CLUSTER(::Poseidon::Logger::LV_INFO,     __VA_ARGS__)
#define LOG_EMPERY_CLUSTER_DEBUG(...)       LOG_EMPERY_CLUSTER(::Poseidon::Logger::LV_DEBUG,    __VA_ARGS__)
#define LOG_EMPERY_CLUSTER_TRACE(...)       LOG_EMPERY_CLUSTER(::Poseidon::Logger::LV_TRACE,    __VA_ARGS__)

#endif
