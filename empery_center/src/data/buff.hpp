#ifndef EMPERY_CENTER_DATA_BUFF_IN_HPP_
#define EMPERY_CENTER_DATA_BUFF_IN_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter {

namespace Data {
	class Buff {
	public:
		static boost::shared_ptr<const Buff> get(BuffId buff_id);
		static boost::shared_ptr<const Buff> require(BuffId buff_id);

	public:
		BuffId buff_id;
		boost::container::flat_map<AttributeId, double> attributes;
	};
}

}

#endif
