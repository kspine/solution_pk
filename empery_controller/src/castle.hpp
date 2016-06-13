#ifndef EMPERY_CONTROLLER_CASTLE_HPP_
#define EMPERY_CONTROLLER_CASTLE_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapObject;
}

}

namespace EmperyController {

class Castle : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<EmperyCenter::MySql::Center_MapObject> m_obj;

public:
	explicit Castle(boost::shared_ptr<EmperyCenter::MySql::Center_MapObject> obj);
	~Castle();

public:
	MapObjectUuid get_map_object_uuid() const;
	MapObjectTypeId get_map_object_type_id() const;
	AccountUuid get_owner_uuid() const;
	MapObjectUuid get_parent_object_uuid() const;

	const std::string &get_name() const;
	Coord get_coord() const;
	std::uint64_t get_created_time() const;
	bool has_been_deleted() const;
	bool is_garrisoned() const;
};

}

#endif
