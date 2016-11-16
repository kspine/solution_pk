  #include "../precompiled.hpp"
  #include "common.hpp"

  #include "../msg/cs_legion_financial.hpp"
  #include "../msg/sc_legion_financial.hpp"

  #include "../msg/err_legion.hpp"

  #include "../legion.hpp"
  #include "../legion_member.hpp"

  #include "../legion_attribute_ids.hpp"
  #include "../legion_member_attribute_ids.hpp"

  #include "../singletons/legion_map.hpp"
  #include "../singletons/legion_member_map.hpp"

  #include "../singletons/legion_financial_map.hpp"

  #include "../item_ids.hpp"

  #include "../mysql/legion.hpp"


namespace EmperyCenter{
    PLAYER_SERVLET(Msg::CS_GetLegionFinancialInfoReqMessage, account, session, req)
    {
      PROFILE_ME;
      const auto account_uuid = account->get_account_uuid();
 
      //判决流程
      const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
      if (!member)
      {
          // LOG_EMPERY_CENTER_ERROR("未加入军团联盟 ERR_LEGION_NOT_JOIN;");
          //未加入军团联盟
          return Response(Msg::ERR_LEGION_NOT_JOIN);
      }
      const auto legion_uuid = LegionUuid(member->get_legion_uuid());
      const auto legion = LegionMap::get(legion_uuid);
      if(!legion)
      {
        // LOG_EMPERY_CENTER_ERROR("军团不存在 ERR_LEGION_CANNOT_FIND;");
        //军团不存在
        return Response(Msg::ERR_LEGION_CANNOT_FIND);
      }

       //回复军团资金账本信息
       std::vector<boost::shared_ptr<MySql::Center_LegionFinancial>> legionfinancials;
       LegionFinancialMap::get_legion_financial(legion_uuid,legionfinancials);

       Msg::SC_GetLegionFinancialInfoReqMessage msg;
       msg.legion_financial_array.reserve(legionfinancials.size());
       for (auto it = legionfinancials.begin(); it != legionfinancials.end(); ++it)
       {
         auto info = (*it);
         auto &elem = *msg.legion_financial_array.emplace(msg.legion_financial_array.end());
         elem.legion_uuid = info->unlocked_get_legion_uuid().to_string();
         elem.account_uuid = info->unlocked_get_account_uuid().to_string();
         elem.item_id = info->unlocked_get_item_id();
         elem.old_count = info->unlocked_get_old_count();
         elem.new_count = info->unlocked_get_new_count();

         elem.action_id = info->unlocked_get_action_id();
         elem.action_count = info->unlocked_get_action_count();
         elem.created_time = info->unlocked_get_created_time();
       }

       session->send(msg);

       return Response(Msg::ST_OK);
    }
}
