#include "precompiled.hpp"
#include "account_utilities.hpp"
#include "mmain.hpp"
#include <poseidon/async_job.hpp>
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "singletons/task_box_map.hpp"
#include "task_box.hpp"
#include "map_object_type_ids.hpp"
#include "task_type_ids.hpp"
#include "singletons/world_map.hpp"
#include "map_cell.hpp"
#include "castle.hpp"
#include "singletons/controller_client.hpp"
#include "msg/st_account.hpp"
#include "account_attribute_ids.hpp"
#include "mail_box.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_data.hpp"
#include "chat_message_type_ids.hpp"
#include "chat_message_slot_ids.hpp"
#include "buff_ids.hpp"

namespace EmperyCenter {

void async_accumulate_promotion_bonus(AccountUuid account_uuid, std::uint64_t taxing_amount) noexcept
try {
	Poseidon::enqueue_async_job(
		[=]() mutable {
			PROFILE_ME;

			const auto controller = ControllerClient::require();

			Msg::ST_AccountAccumulatePromotionBonus treq;
			treq.account_uuid  = account_uuid.str();
			treq.taxing_amount = taxing_amount;
			auto result = controller->send_and_wait(treq);
			LOG_EMPERY_CENTER_INFO("#> Promotion bonus requested: treq = ", treq,
				", code = ", result.first, ", msg = ", result.second);
		});
} catch(std::exception &e){
	LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
}

void async_recheck_building_level_tasks(AccountUuid account_uuid) noexcept
try {
	Poseidon::enqueue_async_job(
		[=]() mutable {
			PROFILE_ME;

			AccountMap::require_controller_token(account_uuid, { });

			const auto task_box = TaskBoxMap::require(account_uuid);

			using BuildingLevelMap = boost::container::flat_map<BuildingId, boost::container::flat_map<unsigned, std::size_t>>;

			BuildingLevelMap building_levels_primary, building_levels_non_primary;

			const auto primary_castle = WorldMap::require_primary_castle(account_uuid);

			std::vector<boost::shared_ptr<MapObject>> map_objects;
			WorldMap::get_map_objects_by_owner(map_objects, account_uuid);
			for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
				const auto &map_object = *it;
				if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
					continue;
				}
				const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
				if(!castle){
					continue;
				}
				if(castle == primary_castle){
					castle->accumulate_building_levels(building_levels_primary);
				} else {
					castle->accumulate_building_levels(building_levels_non_primary);
				}
			}

			const auto accumulate_all = [&](BuildingLevelMap &levels){
				for(auto it = levels.begin(); it != levels.end(); ++it){
					std::size_t virtual_count = 0;
					for(auto lvit = it->second.rbegin(); lvit != it->second.rend(); ++lvit){
						virtual_count += lvit->second;
						lvit->second = virtual_count;
					}
				}
			};

			accumulate_all(building_levels_primary);
			accumulate_all(building_levels_non_primary);

			const auto check_all = [&](const BuildingLevelMap &levels, TaskBox::CastleCategory castle_category){
				for(auto it = levels.begin(); it != levels.end(); ++it){
					for(auto lvit = it->second.rbegin(); lvit != it->second.rend(); ++lvit){
			//			LOG_EMPERY_CENTER_DEBUG("Checking task: account_uuid = ", account_uuid, ", castle_category = ", (unsigned)castle_category,
			//				", building_id = ", it->first, ", count = ", lvit->second, ", level = ", lvit->first);
						task_box->check(TaskTypeIds::ID_UPGRADE_BUILDING_TO_LEVEL, it->first.get(), lvit->second,
							castle_category, lvit->first, 0);
					}
				}
			};

			check_all(building_levels_primary,     TaskBox::TCC_PRIMARY);
			check_all(building_levels_non_primary, TaskBox::TCC_NON_PRIMARY);

			for(auto it = building_levels_non_primary.begin(); it != building_levels_non_primary.end(); ++it){
				for(auto lvit = it->second.rbegin(); lvit != it->second.rend(); ++lvit){
					building_levels_primary[it->first][lvit->first] += lvit->second;
				}
			}
			check_all(building_levels_primary,     TaskBox::TCC_ALL);
		});
} catch(std::exception &e){
	LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
}
void async_cancel_noviciate_protection(AccountUuid account_uuid) noexcept
try {
	PROFILE_ME;

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return;
	}
	const auto protection_expired = account->cast_attribute<bool>(AccountAttributeIds::ID_NOVICIATE_PROTECTION_EXPIRED);
	if(protection_expired){
		return;
	}

	Poseidon::enqueue_async_job(
		[=]() mutable {
			PROFILE_ME;

			AccountMap::require_controller_token(account_uuid, { });

			const auto mail_box = MailBoxMap::require(account_uuid);

			const auto protection_expired = account->cast_attribute<bool>(AccountAttributeIds::ID_NOVICIATE_PROTECTION_EXPIRED);
			if(protection_expired){
				return;
			}

			std::vector<boost::shared_ptr<MapObject>> owning_objects;
			WorldMap::get_map_objects_by_owner(owning_objects, account_uuid);

			std::vector<boost::shared_ptr<MapCell>> owning_cells;
			owning_cells.reserve(owning_objects.size() * 16);
			for(const auto &map_object : owning_objects){
				WorldMap::get_map_cells_by_parent_object(owning_cells, map_object->get_map_object_uuid());
			}

			for(const auto &map_object : owning_objects){
				map_object->clear_buff(BuffIds::ID_NOVICIATE_PROTECTION);
			}
			for(const auto &map_cell : owning_cells){
				map_cell->clear_buff(BuffIds::ID_NOVICIATE_PROTECTION);
			}

			const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
			const auto language_id = LanguageId(); // neutral

			std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
			const auto utc_now = Poseidon::get_utc_time();
			const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
				ChatMessageTypeIds::ID_NOVICIATE_PROTECTION_FINISH, AccountUuid(), std::string(), std::move(segments),
				boost::container::flat_map<ItemId, std::uint64_t>());
			MailBoxMap::insert_mail_data(mail_data);

			MailBox::MailInfo mail_info = { };
			mail_info.mail_uuid   = mail_uuid;
			mail_info.expiry_time = UINT64_MAX;
			mail_info.system      = true;
			mail_box->insert(std::move(mail_info));

			boost::container::flat_map<AccountAttributeId, std::string> modifiers;
			modifiers[AccountAttributeIds::ID_NOVICIATE_PROTECTION_EXPIRED] = "1";
			account->set_attributes(std::move(modifiers));
		});
} catch(std::exception &e){
	LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
}

}
