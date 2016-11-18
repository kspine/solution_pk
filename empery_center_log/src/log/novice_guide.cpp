#include "../precompiled.hpp"
#include "common.hpp"
#include "../../../empery_center/src/msg/sl_novice_guide.hpp"
#include "../mysql/novice_guide.hpp"

namespace EmperyCenterLog {

   LOG_SERVLET(Msg::SL_NoviceGuideTrace, log, req){
	auto obj = boost::make_shared<MySql::CenterLog_NoviceGuideTrace>(
	Poseidon::Uuid(req.account_uuid),
	req.task_id,
	req.step_id,
	req.created_time);
    obj->async_save(false, true);
   }
}