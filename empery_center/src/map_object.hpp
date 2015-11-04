#ifndef EMPERY_CENTER_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MAP_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <utility>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapObject;
	class Center_MapObjectAttribute;
}

class MapObjectMap;

class MapObject : NONCOPYABLE {
	friend MapObjectMap;

private:
	boost::shared_ptr<MySql::Center_MapObject> m_obj;
	boost::container::flat_map<MapObjectAttrId, boost::shared_ptr<MySql::Center_MapObjectAttribute>> m_attributes;

public:
	MapObject(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid);
	MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes);
	virtual ~MapObject();

protected:
	void deleteFromGame();

public:
	MapObjectUuid getMapObjectUuid() const;
	MapObjectTypeId getMapObjectTypeId() const;
	AccountUuid getOwnerUuid() const;
	boost::uint64_t getCreatedTime() const;
	bool hasBeenDeleted() const;

	void getAttributes(boost::container::flat_map<MapObjectAttrId, boost::int64_t> &ret) const;
	boost::int64_t getAttribute(MapObjectAttrId mapObjectAttrId) const;
	void setAttribute(MapObjectAttrId mapObjectAttrId, boost::int64_t value);

	Coord getCoord() const;
};

}

#endif
