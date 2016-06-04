#ifndef EMPERY_CENTER_RESOURCE_UTILITIES_HPP_
#define EMPERY_CENTER_RESOURCE_UTILITIES_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/container/flat_map.hpp>
#include <utility>
#include "id_types.hpp"

namespace EmperyCenter {

class Castle;
class TaskBox;

extern ResourceId try_decrement_resources(const boost::shared_ptr<Castle> &castle, const boost::shared_ptr<TaskBox> &task_box,
	const boost::container::flat_map<ResourceId, std::uint64_t> &resources_to_consume,
	ReasonId reason, std::uint64_t param1, std::uint64_t param2, std::uint64_t param3, const boost::function<void ()> &callback);

}

#endif
