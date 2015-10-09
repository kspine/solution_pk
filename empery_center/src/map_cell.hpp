#ifndef EMPERY_CENTER_MAP_CELL_HPP_
#define EMPERY_CENTER_MAP_CELL_HPP_

#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/flat_map.hpp>
#include <utility>
#include "id_types.hpp"

namespace EmperyCenter {

class Unit;

struct MapCell : public boost::enable_shared_from_this<MapCell> {
	const boost::int64_t x;
	const boost::int64_t y;
	const TerrainId terrainId;
	const OverlayId overlayId;

	std::pair<ResourceId, boost::uint64_t> resource;
	AccountUuid ownerUuid;
	boost::flat_multimap<BuffId, std::pair<AccountUuid, boost::uint64_t>> buffSet;

	boost::weak_ptr<Unit> unit;

	MapCell(boost::int64_t x_, boost::int64_t y_, TerrainId terrainId_, OverlayId overlayId_)
		: x(x_), y(y_), terrainId(terrainId_), overlayId(overlayId_)
	{
	}
};

}

#endif
