#include "../precompiled.hpp"
#include "common.hpp"

#include "../msg/cs_novice_guide.hpp"
#include "../msg/sc_novice_guide.hpp"

#include "../id_types.hpp"


#include "../singletons/item_box_map.hpp"
#include "../singletons/task_box_map.hpp"
#include "../singletons/novice_guide_map.hpp"
#include "../singletons/world_map.hpp"

#include "../item_box.hpp"
#include "../item_ids.hpp"
#include "../reason_ids.hpp"

#include "../task_box.hpp"
#include "../transaction_element.hpp"
#include "../castle.hpp"

#include "../data/novice_guide_setup.hpp"

#include "../mysql/novice_guide.hpp"

#include <poseidon/singletons/mysql_daemon.hpp>

#include "../novice_guide_log.hpp"

namespace EmperyCenter
{
	PLAYER_SERVLET(Msg::CS_GetTaskStepInfoReqMessage, account, session, req)
    {
       PROFILE_ME;
       const auto account_uuid = account->get_account_uuid();
       const auto task_id = req.task_id;
       std::uint64_t  step_id = NoviceGuideMap::get_step_id(account_uuid,TaskId(task_id));
       if(step_id == (std::uint64_t)0)
       {
         step_id = 2;
       }

       LOG_EMPERY_CENTER_ERROR("Msg::CS_GetTaskStepInfoReqMessage2000-2002:taskid:",task_id,"step_id:",step_id);
       Msg::SC_GetTaskStepInfoResMessage msg;
       msg.task_id = task_id;
       msg.step_id = step_id;
       session->send(msg);
       return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_GuideTaskStepReqMessage, account, session, req)
    {
       PROFILE_ME;
       const auto account_uuid = account->get_account_uuid();
       const auto task_id = req.task_id;
       const auto step_id = req.step_id;
       const auto task_box = TaskBoxMap::require(account_uuid);

       LOG_EMPERY_CENTER_ERROR("Msg::CS_GuideTaskStepReqMessage2001:taskid:",task_id,"step_id:",step_id);

       TaskId task_ids = TaskId(task_id);
       std::uint64_t stepid = (std::uint64_t)step_id;


       NoviceGuideMap::make_insert(account_uuid,task_ids,stepid);

       auto pshare = Data::NoviceGuideSetup::require(stepid);

       //item_rewards
       {
         auto item_reward_map = pshare->item_rewards;

         const auto item_box = ItemBoxMap::require(account_uuid);

         std::vector<ItemTransactionElement> transaction;

         for(auto it = item_reward_map.begin();it != item_reward_map.end();++it)
         {
            transaction.emplace_back(ItemTransactionElement::OP_ADD,it->first, it->second,
       								ReasonIds::ID_NOVICE_GUIDE_ADD_ITEM, 0, 0, 0);
         }
         item_box->commit_transaction(transaction, false);
       }

       auto castle = WorldMap::require_primary_castle(account->get_account_uuid());

       //resource_rewards;
       {
         auto resource_rewards = pshare->resource_rewards;
         std::vector<ResourceTransactionElement> transaction;
         transaction.reserve(resource_rewards.size());


         for(auto it = resource_rewards.begin(); it != resource_rewards.end(); ++it)
         {
           transaction.emplace_back(ResourceTransactionElement::OP_ADD, it->first, it->second,
                                    ReasonIds::ID_NOVICE_GUIDE_ADD_RESOURCE, 0, 0, 0);
         }
         castle->commit_resource_transaction(transaction);
       }

       //arm_rewards
       {
         auto arm_rewards = pshare->arm_rewards;
         std::vector<SoldierTransactionElement> transaction;
         transaction.reserve(arm_rewards.size());
        // LOG_EMPERY_CENTER_ERROR("add arm_rewaeds size:",arm_rewards.size());
         for(auto it = arm_rewards.begin(); it != arm_rewards.end(); ++it)
         {
        // 	LOG_EMPERY_CENTER_ERROR("add arm_rewaeds arm_id:",it->first," count:", it->second);
       	   transaction.emplace_back(SoldierTransactionElement::OP_ADD, it->first, it->second,
       			ReasonIds::ID_NOVICE_GUIDE_ADD_ARM,0, 0, 0);
         }
       	 castle->commit_soldier_transaction(transaction);
       }

       LOG_EMPERY_CENTER_FATAL("NoviceGuideLog::NoviceGuideTrace!!!");
       NoviceGuideLog::NoviceGuideTrace(AccountUuid(account_uuid),task_ids,stepid,Poseidon::get_utc_time());

       return Response(Msg::ST_OK);

    }
}