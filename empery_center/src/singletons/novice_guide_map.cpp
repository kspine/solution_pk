#include "../precompiled.hpp"
#include "novice_guide_map.hpp"

#include "../mmain.hpp"

#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>

#include <tuple>

#include "../player_session.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"

#include "../mysql/novice_guide.hpp"

#include "account_map.hpp"
#include "../account_attribute_ids.hpp"

namespace EmperyCenter{
  namespace{
        struct NoviceGuideElement
		{
			boost::shared_ptr<MySql::Center_NoviceGuide> novice_guide;
			AccountUuid account_uuid;
			TaskId task_id;
			std::uint64_t step_id;
		    explicit NoviceGuideElement(boost::shared_ptr<MySql::Center_NoviceGuide> novice_guide_)
		    : novice_guide(std::move(novice_guide_))
			, account_uuid(novice_guide->get_account_uuid())
			, task_id(novice_guide->get_task_id())
			, step_id(novice_guide->get_step_id()){}
		};
		MULTI_INDEX_MAP(NoviceGuideContainer, NoviceGuideElement,
		   MULTI_MEMBER_INDEX(account_uuid)
		   MULTI_MEMBER_INDEX(task_id)
		   MULTI_MEMBER_INDEX(step_id)
		)
		boost::shared_ptr<NoviceGuideContainer> g_novice_guide_map;

		MODULE_RAII_PRIORITY(handles, 5000)
		{
		  const auto conn = Poseidon::MySqlDaemon::create_connection();

		  struct NoviceGuideElement
		  {
		     boost::shared_ptr<MySql::Center_NoviceGuide> obj;
	      };
	      std::map<AccountUuid, NoviceGuideElement> temp_map;

		  LOG_EMPERY_CENTER_INFO("Loading Center_NoviceGuide...");

		  conn->execute_sql("SELECT * FROM `Center_NoviceGuide`");

		  while (conn->fetch_row())
		  {
			 auto obj = boost::make_shared<MySql::Center_NoviceGuide>();
			 obj->fetch(conn);
			 obj->enable_auto_saving();
			 const auto account_uuid = AccountUuid(obj->get_account_uuid());
			 temp_map[account_uuid].obj = std::move(obj);
		  }
		  LOG_EMPERY_CENTER_INFO("Loaded ", temp_map.size(), " novice_guide.");
		  const auto novice_guide_map = boost::make_shared<NoviceGuideContainer>();
		  for (auto it = temp_map.begin(); it != temp_map.end(); ++it)
		  {
			 novice_guide_map->insert(NoviceGuideElement(std::move(it->second.obj)));
		  }
		  g_novice_guide_map = novice_guide_map;
		  handles.push(novice_guide_map);

	    }
	 }

	 void NoviceGuideMap::insert(const boost::shared_ptr<MySql::Center_NoviceGuide> &novice_guide)
	 {
	 	PROFILE_ME;
	 	const auto &novice_guide_map = g_novice_guide_map;
	 	if (!novice_guide_map)
	 	{
	 	   return;
	 	}
	 	if (!novice_guide_map->insert(NoviceGuideElement(novice_guide)).second)
	 	{
	 	   return;
	 	}
	 }

	 boost::shared_ptr<MySql::Center_NoviceGuide> NoviceGuideMap::find(AccountUuid account_uuid,TaskId task_id)
	 {
	       PROFILE_ME;
	 	const auto &novice_guide_map = g_novice_guide_map;
	 	if (!novice_guide_map)
	 	{
	 	   return {};
	 	}
	 	const auto range = novice_guide_map->equal_range<1>(account_uuid);
	 	for(auto it = range.first; it != range.second; ++it){
	 	     if(AccountUuid(it->novice_guide->unlocked_get_account_uuid()) == account_uuid 
	 	          && TaskId(it->novice_guide->unlocked_get_task_id()) == task_id){

	 		    return it->novice_guide;
	 	     }
	    }
	    return { };
	 }


    void NoviceGuideMap::get_step_id(AccountUuid account_uuid,TaskId task_id,std::uint64_t& step_id)
    {
	   PROFILE_ME;
	   const auto  pNoviceGuide = find(account_uuid,task_id);
	   if (NULL != pNoviceGuide)
	   {
		  step_id = static_cast<std::uint64_t>(pNoviceGuide->unlocked_get_step_id());
	   }
	   return;
    }

    bool NoviceGuideMap::check_step_id(AccountUuid account_uuid,TaskId task_id,std::uint64_t step_id)
    {
       NoviceGuideStepId current_step_id;
       get_step_id(account_uuid,task_id,&current_step_id);
       if(current_step_id >= 0 && step_id == current_step_id){
         return true;
       }
       return false;
    }
}