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

            std::pair<std::pair<AccountUuid,TaskId>, std::uint64_t> account_pair;
            std::pair<AccountUuid,TaskId> task_pair;

		    NoviceGuideElement(boost::shared_ptr<MySql::Center_NoviceGuide> novice_guide_)
		    : novice_guide(std::move(novice_guide_))
		    ,account_pair(std::make_pair(std::make_pair(AccountUuid(novice_guide->get_account_uuid()),TaskId(novice_guide->get_task_id())),novice_guide->get_step_id()))
		    ,task_pair(std::make_pair(AccountUuid(novice_guide->get_account_uuid()),TaskId(novice_guide->get_task_id())))
		    {
		    }
		};

	MULTI_INDEX_MAP(NoviceGuideContainer, NoviceGuideElement,
		UNIQUE_MEMBER_INDEX(account_pair)
		MULTI_MEMBER_INDEX(task_pair)
		)
		boost::shared_ptr<NoviceGuideContainer> g_novice_guide_map;


		MODULE_RAII_PRIORITY(handles, 5000)
		{
          const auto novice_guide_map = boost::make_shared<NoviceGuideContainer>();

		  const auto conn = Poseidon::MySqlDaemon::create_connection();

		  LOG_EMPERY_CENTER_INFO("Loading Center_NoviceGuide...");

		  conn->execute_sql("SELECT * FROM `Center_NoviceGuide`");

		  while (conn->fetch_row())
		  {
			 auto obj = boost::make_shared<MySql::Center_NoviceGuide>();
			 obj->fetch(conn);
			 obj->enable_auto_saving();

			 novice_guide_map->insert(NoviceGuideElement(std::move(obj)));
		  }
		  LOG_EMPERY_CENTER_ERROR("Loaded ", novice_guide_map->size(), " novice_guide.");
		  g_novice_guide_map = novice_guide_map;
		  handles.push(novice_guide_map);

	    }
}


void NoviceGuideMap::make_insert(AccountUuid account_uuid,TaskId task_id,std::uint64_t step_id)
	 {
	    PROFILE_ME;

	    auto obj = boost::make_shared<MySql::Center_NoviceGuide>(account_uuid.get(),task_id.get(),step_id);

	    obj->enable_auto_saving();
	    Poseidon::MySqlDaemon::enqueue_for_saving(obj,true, true);

        const auto &novice_guide_map = g_novice_guide_map;
        if (!novice_guide_map)
        {
            return;
        }

       bool bfind = false; 
       const auto range = novice_guide_map->equal_range<1>(std::make_pair(account_uuid,task_id));
	   for(auto it = range.first; it != range.second; ++it)
	   {
	      if(AccountUuid(it->novice_guide->unlocked_get_account_uuid()) == account_uuid &&  TaskId(it->novice_guide->unlocked_get_task_id()) == task_id){

	          it->novice_guide->set_step_id(step_id);
	          bfind  = true;
	          return;
	      }

	   }
	   if(!bfind)
	      novice_guide_map->insert(NoviceGuideElement(std::move(obj)));
	      bfind = false; 
 
        for(auto itt = novice_guide_map->begin<0>(); itt != novice_guide_map->end<0>(); ++itt)
	    {
	        if(AccountUuid(itt->novice_guide->unlocked_get_account_uuid()) == account_uuid){
	        LOG_EMPERY_CENTER_ERROR("insert accountuuid: ", itt->novice_guide->unlocked_get_account_uuid(), 
	                                         "taskid: ",  itt->novice_guide->unlocked_get_task_id(),
	                                         "step_id: ", itt->novice_guide->unlocked_get_step_id());}
	    }
	 }


    std::uint64_t NoviceGuideMap::get_step_id(AccountUuid account_uuid,TaskId task_id)
    {
	   PROFILE_ME;
	   
	   const auto &novice_guide_map = g_novice_guide_map;
	    if (!novice_guide_map)
	    {
	       return 0;
	    }

	   const auto range = novice_guide_map->equal_range<1>(std::make_pair(account_uuid,task_id));
	   for(auto it = range.first; it != range.second; ++it)
	   {
	      if(AccountUuid(it->novice_guide->unlocked_get_account_uuid()) == account_uuid && 
	      TaskId(it->novice_guide->unlocked_get_task_id()) == task_id){

           LOG_EMPERY_CENTER_ERROR("accountuuid: ", it->novice_guide->unlocked_get_account_uuid(), 
                                   "taskid: ",  it->novice_guide->unlocked_get_task_id(),
                                   "step_id: ", it->novice_guide->unlocked_get_step_id());


	         return it->novice_guide->unlocked_get_step_id();
	      }
	   }
	   return 0;
    
    }
}