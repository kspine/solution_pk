#include "../precompiled.hpp"
#include "legion_package_corps_box.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "legion_task_contribution.hpp"

namespace EmperyCenter
{
	namespace
	{
		MULTI_INDEX_MAP(LegionTaskContributionContainer, Data::LegionTaskContribution,
			UNIQUE_MEMBER_INDEX(contribution_id))

			boost::weak_ptr<const LegionTaskContributionContainer> g_legionTaskContribution_container;
		const char CORPS_TASK_CONTRIBUTION_FILE[] = "corps_task_contribution";

		MODULE_RAII_PRIORITY(handles, 1000)
		{
			/*
			auto csv = Data::sync_load_data(CORPS_TASK_CONTRIBUTION_FILE);
			const auto legionTaskContribution_container = boost::make_shared<LegionTaskContributionContainer>();
			while (csv.fetch_row())
			{
				Data::LegionTaskContribution elem = {};

				csv.get(elem.contribution_id, "contribution_id");
				csv.get(elem.contribution_key, "output_number");
				csv.get(elem.contribution_value, "corps_number");

				Poseidon::JsonObject object;

				LOG_EMPERY_CENTER_ERROR("Load  corps_task_contribution.csv : success!!!");
			}
			g_legionTaskContribution_container = legionTaskContribution_container;
			handles.push(legionTaskContribution_container);
			auto servlet = DataSession::create_servlet(CORPS_TASK_CONTRIBUTION_FILE, Data::encode_csv_as_json(csv, "contribution_id"));
			handles.push(std::move(servlet));
			*/

		}
	}

	namespace Data
	{
		boost::shared_ptr<const LegionTaskContribution> LegionTaskContribution::get(LegionTaskContributionId contribution_id)
		{
			PROFILE_ME;

			const auto legionTaskContribution_container = g_legionTaskContribution_container.lock();
			if (!legionTaskContribution_container)
			{
				LOG_EMPERY_CENTER_WARNING("legionTaskContribution_container has not been loaded.");
				return{};
			}

			const auto it = legionTaskContribution_container->find<0>(contribution_id);
			if (it == legionTaskContribution_container->end<0>())
			{
				LOG_EMPERY_CENTER_TRACE("corps_task_contribution not found: contribution_id = ", contribution_id);
				return{};
			}
			return boost::shared_ptr<const LegionTaskContribution>(legionTaskContribution_container, &*it);
		}

		boost::shared_ptr<const LegionTaskContribution> LegionTaskContribution::require(LegionTaskContributionId contribution_id)
		{
			PROFILE_ME;
			auto ret = get(contribution_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("corps_task_contribution not found: contribution_id = ", contribution_id);
				DEBUG_THROW(Exception, sslit("corps_task_contribution not found"));
			}
			return ret;
		}
	}
}