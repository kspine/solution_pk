#ifndef EMPERY_CENTER_LEGION_BUILDING_HPP_
#define EMPERY_CENTER_LEGION_BUILDING_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include "id_types.hpp"
#include "legion_building_attribute_ids.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_LegionBuilding;
	class Center_LegionBuildingAttribute;
}

class PlayerSession;

class LegionBuilding : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:

	static std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<LegionBuilding>> async_create(
		 LegionBuildingUuid legion_building_uuid,  LegionUuid legion_uuid,  MapObjectUuid map_object_uuid, std::uint64_t ntype);

private:
	const boost::shared_ptr<MySql::Center_LegionBuilding> m_obj;

	boost::container::flat_map<LegionBuildingAttributeId,
		boost::shared_ptr<MySql::Center_LegionBuildingAttribute>> m_attributes;


public:
	LegionBuilding(boost::shared_ptr<MySql::Center_LegionBuilding> obj,
		const std::vector<boost::shared_ptr<MySql::Center_LegionBuildingAttribute>> &attributes);
	~LegionBuilding();

public:
	LegionBuildingUuid get_legion_building_uuid() const;

	LegionUuid get_legion_uuid() const;

	MapObjectUuid get_map_object_uuid()  const;

	std::uint64_t get_type() const;

	// 初始化属性
	void InitAttributes(std::string strobjuuid);
	// 离开善后操作
	void leave();

	const std::string &get_attribute(LegionBuildingAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LegionBuildingAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LegionBuildingAttributeId, std::string> modifiers);

	template<typename T, typename DefaultT = T>
	T cast_attribute(LegionBuildingAttributeId account_attribute_id, const DefaultT def = DefaultT()){
		const auto &str = get_attribute(account_attribute_id);
		if(str.empty()){
			return def;
		}
		return boost::lexical_cast<T>(str);
	}
};

}

#endif
