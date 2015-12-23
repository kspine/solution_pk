#include "precompiled.hpp"
#include "utilities.hpp"

#define EMPERY_CENTER_UTILITIES_NAMESPACE_  EmperyCenter
#include "utilities_impl.cc"

#include <poseidon/json.hpp>
#include <poseidon/string.hpp>
#include <poseidon/async_job.hpp>
#include "cbpp_response.hpp"
#include "data/global.hpp"
#include "data/map.hpp"
#include "map_object.hpp"
#include "overlay.hpp"
#include "singletons/world_map.hpp"
#include "msg/err_map.hpp"
#include "map_object_type_ids.hpp"
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_box.hpp"
#include "mail_data.hpp"
#include "item_ids.hpp"
#include "mail_type_ids.hpp"
#include "chat_message_slot_ids.hpp"

namespace EmperyCenter {

std::pair<long, std::string> can_deploy_castle_at(Coord coord, const boost::shared_ptr<MapObject> &excluding_map_object){
	PROFILE_ME;

	using Response = CbppResponse;

	std::vector<boost::shared_ptr<MapObject>> other_map_objects;

	std::vector<Coord> foundation;
	get_castle_foundation(foundation, coord, true);
	for(auto it = foundation.begin(); it != foundation.end(); ++it){
		const auto &foundation_coord = *it;
		const auto cluster_scope = WorldMap::get_cluster_scope(foundation_coord);
		const auto map_x = static_cast<unsigned>(foundation_coord.x() - cluster_scope.left());
		const auto map_y = static_cast<unsigned>(foundation_coord.y() - cluster_scope.bottom());
		LOG_EMPERY_CENTER_DEBUG("Castle foundation: foundation_coord = ", foundation_coord, ", cluster_scope = ", cluster_scope,
			", map_x = ", map_x, ", map_y = ", map_y);
		const auto basic_data = Data::MapCellBasic::require(map_x, map_y);
		const auto terrain_data = Data::MapTerrain::require(basic_data->terrain_id);
		if(!terrain_data->buildable){
			return Response(Msg::ERR_CANNOT_DEPLOY_IMMIGRANTS_HERE) <<foundation_coord;
		}

		std::vector<boost::shared_ptr<Overlay>> overlays;
		WorldMap::get_overlays_by_rectangle(overlays, Rectangle(foundation_coord, 1, 1));
		for(auto it = overlays.begin(); it != overlays.end(); ++it){
			const auto &overlay = *it;
			if(!overlay->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_OVERLAY) <<foundation_coord;
			}
		}

		other_map_objects.clear();
		WorldMap::get_map_objects_by_rectangle(other_map_objects, Rectangle(foundation_coord, 1, 1));
		for(auto oit = other_map_objects.begin(); oit != other_map_objects.end(); ++oit){
			const auto &other_object = *oit;
			if(other_object == excluding_map_object){
				continue;
			}
			if(!other_object->is_virtually_removed()){
				return Response(Msg::ERR_CANNOT_DEPLOY_ON_TROOPS) <<foundation_coord;
			}
		}
	}
	// 检测与其他城堡距离。
	const auto min_distance  = (boost::uint32_t)Data::Global::as_unsigned(Data::Global::SLOT_MINIMUM_DISTANCE_BETWEEN_CASTLES);

	const auto cluster_scope = WorldMap::get_cluster_scope(coord);
	const auto other_left     = std::max(coord.x() - (min_distance - 1), cluster_scope.left());
	const auto other_bottom   = std::max(coord.y() - (min_distance - 1), cluster_scope.bottom());
	const auto other_right    = std::min(coord.x() + (min_distance + 2), cluster_scope.right());
	const auto other_top      = std::min(coord.y() + (min_distance + 2), cluster_scope.top());
	other_map_objects.clear();
	WorldMap::get_map_objects_by_rectangle(other_map_objects, Rectangle(Coord(other_left, other_bottom), Coord(other_right, other_top)));
	for(auto it = other_map_objects.begin(); it != other_map_objects.end(); ++it){
		const auto &other_object = *it;
		const auto other_object_type_id = other_object->get_map_object_type_id();
		if(other_object_type_id != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto other_coord = other_object->get_coord();
		const auto other_object_uuid = other_object->get_map_object_uuid();
		LOG_EMPERY_CENTER_DEBUG("Checking distance: other_coord = ", other_coord, ", other_object_uuid = ", other_object_uuid);
		const auto distance = get_distance_of_coords(other_coord, coord);
		if(distance <= min_distance){
			return Response(Msg::ERR_TOO_CLOSE_TO_ANOTHER_CASTLE) <<other_object_uuid;
		}
	}

	return Response();
}

namespace {
	double              g_bonus_ratio;
	std::vector<double> g_level_bonus_array;
	std::vector<double> g_income_tax_array;
	std::vector<double> g_level_bonus_extra_array;
}

MODULE_RAII_PRIORITY(/* handles */, 1000){
	const auto promotion_bonus_ratio = get_config<double>("promotion_bonus_ratio", 0.60);
	LOG_EMPERY_CENTER_DEBUG("> promotion_bonus_ratio = ", promotion_bonus_ratio);
	g_bonus_ratio = promotion_bonus_ratio;

	const auto promotion_level_bonus_array = get_config<Poseidon::JsonArray>("promotion_level_bonus_array");
	std::vector<double> temp_vec;
	temp_vec.reserve(promotion_level_bonus_array.size());
	for(auto it = promotion_level_bonus_array.begin(); it != promotion_level_bonus_array.end(); ++it){
		temp_vec.emplace_back(it->get<double>());
	}
	LOG_EMPERY_CENTER_DEBUG("> promotion_level_bonus_array = ", Poseidon::implode(',', temp_vec));
	g_level_bonus_array = std::move(temp_vec);

	const auto promotion_income_tax_array = get_config<Poseidon::JsonArray>("promotion_income_tax_array");
	temp_vec.clear();
	temp_vec.reserve(promotion_income_tax_array.size());
	for(auto it = promotion_income_tax_array.begin(); it != promotion_income_tax_array.end(); ++it){
		temp_vec.emplace_back(it->get<double>());
	}
	LOG_EMPERY_CENTER_DEBUG("> promotion_income_tax_array = ", Poseidon::implode(',', temp_vec));
	g_income_tax_array = std::move(temp_vec);

	const auto promotion_level_bonus_extra_array = get_config<Poseidon::JsonArray>("promotion_level_bonus_extra_array");
	temp_vec.clear();
	temp_vec.reserve(promotion_level_bonus_extra_array.size());
	for(auto it = promotion_level_bonus_extra_array.begin(); it != promotion_level_bonus_extra_array.end(); ++it){
		temp_vec.emplace_back(it->get<double>());
	}
	LOG_EMPERY_CENTER_DEBUG("> promotion_level_bonus_extra_array = ", Poseidon::implode(',', temp_vec));
	g_level_bonus_extra_array = std::move(temp_vec);
}

void accumulate_promotion_bonus(AccountUuid account_uuid, boost::uint64_t amount){
	PROFILE_ME;

	const auto send_mail_nothrow = [](const boost::shared_ptr<Account> &referrer, MailTypeId mail_type_id,
		boost::uint64_t amount_to_add, const boost::shared_ptr<Account> &taxer) noexcept
	{
		try {
			const auto referrer_uuid = referrer->get_account_uuid();
			const auto mail_box = MailBoxMap::require(referrer_uuid);

			const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
			const auto language_id = LanguageId(); // neutral

			const auto default_mail_expiry_duration = Data::Global::as_unsigned(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION);
			const auto expiry_duration = checked_mul(default_mail_expiry_duration, (boost::uint64_t)60000);
			const auto utc_now = Poseidon::get_utc_time();

			const auto taxer_uuid = taxer->get_account_uuid();

			std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
			segments.emplace_back(ChatMessageSlotIds::ID_TAXER, taxer_uuid.str());

			boost::container::flat_map<ItemId, boost::uint64_t> attachments;
			attachments.emplace(ItemIds::ID_GOLD, amount_to_add);

			const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
				mail_type_id, AccountUuid(), std::string(), std::move(segments), std::move(attachments));
			MailBoxMap::insert_mail_data(mail_data);
			mail_box->insert(mail_data, saturated_add(utc_now, expiry_duration), MailBox::FL_SYSTEM);
			LOG_EMPERY_CENTER_DEBUG("Promotion bonus mail sent: referrer_uuid = ", referrer_uuid,
				", mail_type_id = ", mail_type_id, ", taxer_uuid = ", taxer_uuid, ", amount_to_add = ", amount_to_add);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	};

	const auto withdrawn = boost::make_shared<bool>(true);

	const auto account = AccountMap::require(account_uuid);

	const auto dividend_total         = static_cast<boost::uint64_t>(amount * g_bonus_ratio);
	const auto income_tax_ratio_total = std::accumulate(g_income_tax_array.begin(), g_income_tax_array.end(), 0.0);

	std::vector<boost::shared_ptr<Account>> referrers;
	{
		auto referrer = AccountMap::get(account->get_referrer_uuid());
		while(referrer){
			auto next_referrer = AccountMap::get(referrer->get_referrer_uuid());
			referrers.emplace_back(std::move(referrer));
			referrer = std::move(next_referrer);
		}
	}

	boost::uint64_t dividend_accumulated = 0;

	for(auto it = referrers.begin(); it != referrers.end(); ++it){
		const auto &referrer = *it;

		const auto referrer_uuid       = referrer->get_account_uuid();
		const unsigned promotion_level = referrer->get_promotion_level();

		double my_ratio;
		if((promotion_level == 0) || g_level_bonus_array.empty()){
			my_ratio = 0;
		} else if(promotion_level < g_level_bonus_array.size()){
			my_ratio = g_level_bonus_array.at(promotion_level - 1);
		} else {
			my_ratio = g_level_bonus_array.back();
		}

		const auto my_max_dividend = my_ratio * dividend_total;
		LOG_EMPERY_CENTER_DEBUG("> Current referrer: referrer_uuid = ", referrer_uuid,
			", promotion_level = ", promotion_level, ", my_ratio = ", my_ratio,
			", dividend_accumulated = ", dividend_accumulated, ", my_max_dividend = ", my_max_dividend);

		// 级差制。
		if(dividend_accumulated >= my_max_dividend){
			continue;
		}
		const auto my_dividend = my_max_dividend - dividend_accumulated;
		dividend_accumulated = my_max_dividend;

		// 扣除个税即使没有上家（视为被系统回收）。
		const auto tax_total = static_cast<boost::uint64_t>(std::round(my_dividend * income_tax_ratio_total));
		Poseidon::enqueue_async_job(
			std::bind(send_mail_nothrow, referrer, MailTypeIds::ID_LEVEL_BONUS, my_dividend - tax_total, account),
			{ }, withdrawn);

		{
			auto tit = g_income_tax_array.begin();
			for(auto bit = it + 1; (tit != g_income_tax_array.end()) && (bit != referrers.end()); ++bit){
				const auto &next_referrer = *bit;
				if(next_referrer->get_promotion_level() <= 1){
					continue;
				}
				const auto tax_amount = static_cast<boost::uint64_t>(std::round(my_dividend * (*tit)));
				Poseidon::enqueue_async_job(
					std::bind(send_mail_nothrow, next_referrer, MailTypeIds::ID_INCOME_TAX, tax_amount, account),
					{ }, withdrawn);
				++tit;
			}
		}

		if(promotion_level == g_level_bonus_array.size()){
			auto eit = g_level_bonus_extra_array.begin();
			for(auto bit = it + 1; (eit != g_level_bonus_extra_array.end()) && (bit != referrers.end()); ++bit){
				const auto &next_referrer = *bit;
				if(next_referrer->get_promotion_level() != g_level_bonus_array.size()){
					continue;
				}
				const auto extra_amount = static_cast<boost::uint64_t>(std::round(dividend_total * (*eit)));
				Poseidon::enqueue_async_job(
					std::bind(send_mail_nothrow, next_referrer, MailTypeIds::ID_LEVEL_BONUS_EXTRA, extra_amount, account),
					{ }, withdrawn);
				++eit;
			}
		}
	}

	*withdrawn = false;
}

}
