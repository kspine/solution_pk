#include "../precompiled.hpp"
#include "novice_guide_setup.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
	namespace
	{
		MULTI_INDEX_MAP(NoviceGuideSetupContainer, Data::NoviceGuideSetup,
			UNIQUE_MEMBER_INDEX(step_id))

		boost::weak_ptr<const NoviceGuideSetupContainer> g_noviceGuideSetup_container;
		const char NOVICE_GUIDE_SETUP_FILE[] = "novice_guide_setup";

		MODULE_RAII_PRIORITY(handles, 1000)
		{
			auto csv = Data::sync_load_data(NOVICE_GUIDE_SETUP_FILE);
			const auto noviceGuideSetup_container = boost::make_shared<NoviceGuideSetupContainer>();
			while (csv.fetch_row())
			{
				Data::NoviceGuideSetup elem = {};

				csv.get(elem.step_id, "stepid");

				Poseidon::JsonObject object;

				csv.get(object, "resource");
				elem.resource_rewards.reserve(object.size());
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.resource_rewards.emplace(resource_id, count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate resource reward: resource_id = ", resource_id);
						DEBUG_THROW(Exception, sslit("Duplicate resource  reward"));
					}
				}

				csv.get(object, "arm");
				elem.arm_rewards.reserve(object.size());
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto arm_id = boost::lexical_cast<MapObjectTypeId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.arm_rewards.emplace(arm_id, count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate arm reward: arm_id = ", arm_id);
						DEBUG_THROW(Exception, sslit("Duplicate arm  reward"));
					}
				}

				csv.get(object, "item");
				elem.item_rewards.reserve(object.size());
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto item_id = boost::lexical_cast<ItemId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.item_rewards.emplace(item_id, count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate item reward: item_id = ", item_id);
						DEBUG_THROW(Exception, sslit("Duplicate item  reward"));
					}
				}

				if(!noviceGuideSetup_container->insert(std::move(elem)).second)
				{
				    LOG_EMPERY_CENTER_ERROR("Duplicate noviceGuideSetup: stepid = ", elem.step_id);
				    DEBUG_THROW(Exception, sslit("Duplicate noviceGuideSetup"));
				}
			}

			g_noviceGuideSetup_container = noviceGuideSetup_container;
			handles.push(noviceGuideSetup_container);
			auto servlet = DataSession::create_servlet(NOVICE_GUIDE_SETUP_FILE, Data::encode_csv_as_json(csv, "stepid"));
			handles.push(std::move(servlet));
		}
	}

	namespace Data
	{
		boost::shared_ptr<const NoviceGuideSetup> NoviceGuideSetup::get(std::uint64_t step_id)
		{
			PROFILE_ME;

			const auto noviceGuideSetup_container = g_noviceGuideSetup_container.lock();
			if (!noviceGuideSetup_container)
			{
				LOG_EMPERY_CENTER_WARNING("noviceGuideSetup has not been loaded.");
				return{};
			}

			const auto it = noviceGuideSetup_container->find<0>(step_id);
			if (it == noviceGuideSetup_container->end<0>())
			{
				LOG_EMPERY_CENTER_TRACE("noviceGuideSetup not found: step_id = ", step_id);
				return{};
			}
			return boost::shared_ptr<const NoviceGuideSetup>(noviceGuideSetup_container, &*it);
		}

		boost::shared_ptr<const NoviceGuideSetup> NoviceGuideSetup::require(std::uint64_t step_id)
		{
			PROFILE_ME;
			auto ret = get(step_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("NoviceGuideSetup not found: step_id = ", step_id);
				DEBUG_THROW(Exception, sslit("NoviceGuideSetup not found"));
				return {};
			}
			return ret;
		}
	}
}