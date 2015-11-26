#ifndef EMPERY_CLUSTER_ABSTRACT_DATA_OBJECT_HPP_
#define EMPERY_CLUSTER_ABSTRACT_DATA_OBJECT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>

namespace EmperyCluster {

class AbstractDataObject : public virtual Poseidon::VirtualSharedFromThis {
public:
	AbstractDataObject() = default;
	virtual ~AbstractDataObject() = 0;

	AbstractDataObject(const AbstractDataObject &) = delete;
	AbstractDataObject &operator=(const AbstractDataObject &) = delete;

public:
	virtual void pump_status() = 0;
};

}

#endif
