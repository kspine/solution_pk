#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/st_map.hpp"
#include "../../../empery_center/src/msg/ts_map.hpp"
#include "../../../empery_center/src/msg/err_map.hpp"
#include "../castle.hpp"
#include "../singletons/world_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/async_job.hpp>

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_MapRegisterMapServer, controller, req){
	const auto center_rectangle = WorldMap::get_cluster_scope(Coord(0, 0));
	const auto map_width  = static_cast<std::uint32_t>(center_rectangle.width());
	const auto map_height = static_cast<std::uint32_t>(center_rectangle.height());

	const auto num_coord = Coord(req.numerical_x, req.numerical_y);
	const auto cluster_scope = WorldMap::get_cluster_scope(Coord(num_coord.x() * map_width, num_coord.y() * map_height));
	const auto controller_coord = cluster_scope.bottom_left();
	LOG_EMPERY_CONTROLLER_INFO("Registering map server: num_coord = ", num_coord, ", cluster_scope = ", cluster_scope);

	const auto old_controller = WorldMap::get_controller(controller_coord);
	if(old_controller != controller){
//		if(old_controller){
//			return Response(Msg::ERR_MAP_SERVER_CONFLICT) <<old_controller->get_remote_info().ip;
//		}
		WorldMap::set_controller(controller, controller_coord);
	}

	return Response();
}

namespace {
	void invalidate_castle_aux(void *old_controller, MapObjectUuid map_object_uuid, Coord coord_expected){
		PROFILE_ME;

		const auto delay = get_config<std::uint64_t>("mysql_synchronization_delay", 5000);
		const auto retry_count = get_config<unsigned>("mysql_synchronization_retry_count", 12);

		unsigned count = 0;
		for(;;){
			LOG_EMPERY_CONTROLLER_DEBUG("Try loading castle: map_object_uuid = ", map_object_uuid);
			try {
				const auto castle = WorldMap::forced_reload_castle(map_object_uuid);
				if(!castle){
					LOG_EMPERY_CONTROLLER_DEBUG("Failed to load castle: map_object_uuid = ", map_object_uuid);
					goto _retry;
				}
				const auto new_coord = castle->get_coord();
				LOG_EMPERY_CONTROLLER_DEBUG("Loaded castle: map_object_uuid = ", map_object_uuid,
					", new_coord = ", new_coord, ", coord_expected = ", coord_expected);
				if(new_coord != coord_expected){
					LOG_EMPERY_CONTROLLER_DEBUG("Coord mismatch: map_object_uuid = ", map_object_uuid,
						", new_coord = ", new_coord, ", coord_expected = ", coord_expected);
					goto _retry;
				}
				const auto new_controller = WorldMap::get_controller(new_coord);
				if(!new_controller){
					LOG_EMPERY_CONTROLLER_DEBUG("No controller? new_coord = ", new_coord);
					goto _retry;
				}
				if(new_controller.get() != old_controller){
					Msg::TS_MapInvalidateCastle sreq;
					sreq.map_object_uuid = map_object_uuid.str();
					sreq.coord_x         = new_coord.x();
					sreq.coord_y         = new_coord.y();
					auto sresult = new_controller->send_and_wait(sreq);
					LOG_EMPERY_CONTROLLER_DEBUG("!> Result: code = ", sresult.first, ", msg = ", sresult.second);
					if(sresult.first != Msg::ST_OK){
						goto _retry;
					}
				}
				break;
			} catch(std::exception &e){
				LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
			}
		_retry:
			;

			if(count >= retry_count){
				LOG_EMPERY_CONTROLLER_ERROR("Failed to load castle: map_object_uuid = ", map_object_uuid);
				break;
			}
			LOG_EMPERY_CONTROLLER_DEBUG("Retrying: count = ", count, ", retry_count = ", retry_count);
			++count;

			LOG_EMPERY_CONTROLLER_DEBUG("Preparing to load castle: map_object_uuid = ", map_object_uuid, ", delay = ", delay);
			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			const auto timer = Poseidon::TimerDaemon::register_timer(delay, 0, std::bind([&]{ promise->set_success(); }));
			Poseidon::JobDispatcher::yield(promise, true);
		}
	}
}

CONTROLLER_SERVLET(Msg::ST_MapInvalidateCastle, controller, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);
	const auto coord_expected = Coord(req.coord_x, req.coord_y);

	const auto old_castle = WorldMap::get_castle(map_object_uuid);
	if(!old_castle || (old_castle->get_coord() != coord_expected)){
		Poseidon::enqueue_async_job(std::bind(&invalidate_castle_aux, controller.get(), map_object_uuid, coord_expected));
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::ST_MapRemoveMapObject, controller, req){
	const auto map_object_uuid = MapObjectUuid(req.map_object_uuid);

	boost::container::flat_map<Coord, boost::shared_ptr<ControllerSession>> controllers;
	WorldMap::get_all_controllers(controllers);
	for(auto it = controllers.begin(); it != controllers.end(); ++it){
		const auto &controller = it->second;
		try {
			Msg::TS_MapRemoveMapObject msg;
			msg.map_object_uuid = map_object_uuid.str();
			controller->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
			controller->shutdown(e.what());
		}
	}

	return Response();
}

}
