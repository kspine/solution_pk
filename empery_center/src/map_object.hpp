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
	boost::container::flat_map<AttributeId, boost::shared_ptr<MySql::Center_MapObjectAttribute>> m_attributes;

public:
	MapObject(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid, std::string name);
	MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes);
	virtual ~MapObject();

protected:
	void deleteFromGame();

public:
	MapObjectUuid getMapObjectUuid() const;
	MapObjectTypeId getMapObjectTypeId() const;
	AccountUuid getOwnerUuid() const;
	const std::string &getName() const;
	void setName(std::string name);
	boost::uint64_t getCreatedTime() const;
	bool hasBeenDeleted() const;

	void getAttributes(boost::container::flat_map<AttributeId, boost::int64_t> &ret) const;
	boost::int64_t getAttribute(AttributeId mapObjectAttrId) const;
	void setAttribute(AttributeId mapObjectAttrId, boost::int64_t value);

	Coord getCoord() const;
};

}

#endif
