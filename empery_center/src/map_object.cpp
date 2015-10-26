#include "precompiled.hpp"
#include "map_object.hpp"
#include "mysql/map_object.hpp"

namespace EmperyCenter {
/*
MapObject::MapObject(MapObjectTypeId mapObjectTypeId, const AccountUuid &ownerUuid, boost::uint64_t hitPoints,
	boost::int64_t x, boost::int64_t y, boost::container::flat_map<unsigned, boost::uint64_t> attributes)
	: m_obj([&]{
		auto obj = boost::make_shared<MySql::Center_MapObject>();
		obj->
{
}

#ifndef EMPERY_CENTER_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MAP_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <utility>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace MySql {
    class Center_MapObject;
    }
    
    class MapObject : NONCOPYABLE {
    private:
        const boost::shared_ptr<MySql::Center_MapObject> m_obj;
        
            boost::container::flat_map<unsigned, boost::uint64_t> m_attributes;
            
            public:
                MapObject(boost::shared_ptr<MySql::Center_MapObject> obj);
                    ~MapObject();
                    
                    public:
                        MapObjectUuid getMapObjectUuid() const;
                            MapObjectTypeId getMapObjectTypeId() const;
                            
                                AccountUuid getOwnerUuid() const;
                                    void setOwnerUuid(const AccountUuid &ownerUuid);
                                    
                                        boost::uint64_t getHitPoints() const;
                                            void setHitPoints(boost::uint64_t hitPoints);
                                            
                                                std::pair<boost::int64_t, boost::int64_t> getCoord() const;
                                                    void setCoord(boost::int64_t x, boost::int64_t y);
                                                    
                                                        const boost::container::flat_map<unsigned, boost::uint64_t> &getAttributes() const;
                                                            boost::uint64_t getAttribute(unsigned slot) const;
                                                                void setAttribute(unsigned slot, boost::uint64_t value);
                                                                };
                                                                
                                                                }
                                                                
                                                                #endif
                                                                
*/
}
