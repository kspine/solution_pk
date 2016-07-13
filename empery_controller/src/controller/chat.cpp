#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../../../empery_center/src/msg/st_chat.hpp"
#include "../../../empery_center/src/msg/ts_chat.hpp"
#include "../../../empery_center/src/msg/err_chat.hpp"
#include "../singletons/world_map.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/async_job.hpp>

namespace EmperyController {

namespace {
	void synchronize_horn_message_aux(const boost::weak_ptr<ControllerSession> &weak_controller, HornMessageUuid horn_message_uuid){
		PROFILE_ME;

		const auto delay = get_config<std::uint64_t>("mysql_synchronization_delay", 5000);
		const auto retry_count = get_config<unsigned>("mysql_synchronization_retry_count", 12);

		unsigned count = 0;
		for(;;){
			LOG_EMPERY_CONTROLLER_DEBUG("Synchronising horn message: horn_message_uuid = ", horn_message_uuid);
			try {
				const auto controller = weak_controller.lock();
				if(!controller){
					break;
				}
				Msg::TS_ChatBroadcastHornMessage sreq;
				sreq.horn_message_uuid = horn_message_uuid.str();
				auto sresult = controller->send_and_wait(sreq);
				LOG_EMPERY_CONTROLLER_DEBUG("!> Result: code = ", sresult.first, ", msg = ", sresult.second);
				if(sresult.first != Msg::ST_OK){
					goto _retry;
				}
				break;
			} catch(std::exception &e){
				LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
			}
		_retry:
			;
			if(count >= retry_count){
				LOG_EMPERY_CONTROLLER_ERROR("Failed to synchronize horn message: horn_message_uuid = ", horn_message_uuid);
				break;
			}
			LOG_EMPERY_CONTROLLER_DEBUG("Retrying: count = ", count, ", retry_count = ", retry_count);
			++count;

			LOG_EMPERY_CONTROLLER_DEBUG("Preparing to synchronize horn message: horn_message_uuid = ", horn_message_uuid, ", delay = ", delay);
			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			const auto timer = Poseidon::TimerDaemon::register_timer(delay, 0, std::bind([&]{ promise->set_success(); }));
			Poseidon::JobDispatcher::yield(promise, true);
		}
	}
}

CONTROLLER_SERVLET(Msg::ST_ChatBroadcastHornMessage, controller, req){
	const auto horn_message_uuid = HornMessageUuid(req.horn_message_uuid);

	std::vector<std::pair<Coord, boost::shared_ptr<ControllerSession>>> controllers;
	WorldMap::get_all_controllers(controllers);
	for(auto it = controllers.begin(); it != controllers.end(); ++it){
		const auto &other_controller = it->second;
		try {
			Poseidon::enqueue_async_job(std::bind(&synchronize_horn_message_aux,
				boost::weak_ptr<ControllerSession>(other_controller), horn_message_uuid));
		} catch(std::exception &e){
			LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
		}
	}

	return Response();
}

}
