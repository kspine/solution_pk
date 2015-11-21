#ifndef EMPERY_CENTER_ABSTRACT_DATA_OBJECT_HPP_
#define EMPERY_CENTER_ABSTRACT_DATA_OBJECT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>

namespace EmperyCenter {

class PlayerSession;

class AbstractDataObject : public virtual Poseidon::VirtualSharedFromThis {
public:
	AbstractDataObject() = default;
	virtual ~AbstractDataObject() = 0;

	AbstractDataObject(const AbstractDataObject &) = delete;
	AbstractDataObject &operator=(const AbstractDataObject &) = delete;

public:
	virtual void pump_status() = 0;
	virtual void synchronize_with_client(const boost::shared_ptr<PlayerSession> &session) const = 0;
};

}

#endif
