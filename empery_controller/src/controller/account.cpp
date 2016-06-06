#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/st_account.hpp"
#include "../../../empery_center/src/msg/ts_account.hpp"
#include "../../../empery_center/src/msg/err_account.hpp"
#include "../account.hpp"
#include "../singletons/account_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>

namespace EmperyController {

CONTROLLER_SERVLET(Msg::ST_AccountAcquireToken, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::reload(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Acquire token: controller = ", controller->get_remote_info(), ", account_uuid = ", account_uuid);
	const auto old_controller = account->get_controller();
	if(old_controller != controller){
		if(old_controller){
			const auto invalidation_delay = get_config<std::uint64_t>("account_invalidation_delay", 10000);

			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			const auto timer = Poseidon::TimerDaemon::register_timer(invalidation_delay, 0, std::bind([=]{ promise->set_success(); }));
			LOG_EMPERY_CONTROLLER_DEBUG("Waiting for account invalidation...");
			Poseidon::JobDispatcher::yield(promise, false);

			try {
				Msg::TS_AccountInvalidate msg;
				msg.account_uuid = account_uuid.str();
				old_controller->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CONTROLLER_WARNING("std::exception thrown: what = ", e.what());
				// old_controller->shutdown(e.what());
				throw;
			}
		}
		account->set_controller(controller);
	}

	return Response();
}

CONTROLLER_SERVLET(Msg::ST_AccountQueryToken, controller, req){
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto old_controller = account->get_controller();
	if(old_controller != controller){
		return Response(Msg::ERR_CONTROLLER_TOKEN_NOT_ACQUIRED);
	}

	return Response();
}

}
