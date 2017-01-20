#ifndef EMPERY_CENTER_SINGLETONS_LEGION_DONATE_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_DONATE_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class LegionDonateBox;

struct LegionDonateBoxMap {
	static boost::shared_ptr<LegionDonateBox> get(LegionUuid legion_uuid);
	static boost::shared_ptr<LegionDonateBox> require(LegionUuid legion_uuid);
	static void unload(LegionUuid legion_uuid);

private:
	LegionDonateBoxMap() = delete;
};

}

#endif
