#include "../precompiled.hpp"
#include "global_status.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/async_job.hpp>
#include "../mysql/global_status.hpp"
#include "../utilities.hpp"
#include "../data/promotion.hpp"
#include "account_map.hpp"

namespace EmperyPromotion {

namespace {
	using StatusMap = boost::container::flat_map<unsigned, boost::shared_ptr<MySql::Promotion_GlobalStatus>>;

	boost::shared_ptr<StatusMap> g_status_map;

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto status_map = boost::make_shared<StatusMap>();
		status_map->reserve(100);
		LOG_EMPERY_PROMOTION_INFO("Loading global status...");
		conn->execute_sql("SELECT * FROM `Promotion_GlobalStatus`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			status_map->emplace(obj->get_slot(), std::move(obj));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", status_map->size(), " global status value(s).");
		g_status_map = status_map;
		handles.push(status_map);

		const auto daily_reset_at_o_clock = get_config<unsigned>("daily_reset_at_o_clock", 8);
		auto timer = Poseidon::TimerDaemon::register_daily_timer(daily_reset_at_o_clock, 0, 10, // 推迟十秒钟。
			boost::bind(&GlobalStatus::check_daily_reset));
		handles.push(std::move(timer));

		const auto utc_now = Poseidon::get_utc_time();
		auto first_balancing_time = Poseidon::scan_time(
		                          get_config<std::string>("first_balancing_time").c_str());
		const auto it = status_map->find(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
		if(it != status_map->end()){
			first_balancing_time = it->second->get_value();
		}
		if(utc_now < first_balancing_time){
			timer = Poseidon::TimerDaemon::register_timer(first_balancing_time - utc_now + 10000, 0, // 推迟十秒钟。
				boost::bind(&GlobalStatus::check_daily_reset));
			handles.push(std::move(timer));
		}

		Poseidon::enqueue_async_job(GlobalStatus::check_daily_reset);
	}
}

boost::uint64_t GlobalStatus::get(unsigned slot){
	PROFILE_ME;

	const auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		LOG_EMPERY_PROMOTION_DEBUG("Global status value not found: slot = ", slot);
		return 0;
	}
	return it->second->get_value();
}
boost::uint64_t GlobalStatus::set(unsigned slot, boost::uint64_t new_value){
	PROFILE_ME;

	auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>(slot, 0);
		obj->async_save(true);
		it = g_status_map->emplace_hint(it, slot, std::move(obj));
	}
	const auto old_value = it->second->get_value();
	it->second->set_value(new_value);
	return old_value;
}
boost::uint64_t GlobalStatus::fetch_add(unsigned slot, boost::uint64_t delta_value){
	PROFILE_ME;

	auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>(slot, 0);
		obj->async_save(true);
		it = g_status_map->emplace_hint(it, slot, std::move(obj));
	}
	const auto old_value = it->second->get_value();
	it->second->set_value(old_value + delta_value);
	return old_value;
}

void GlobalStatus::check_daily_reset(){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION_INFO("Checking for global status daily reset...");

	const auto get_obj = [](unsigned slot){
		auto it = g_status_map->find(slot);
		if(it == g_status_map->end()){
			auto obj = boost::make_shared<MySql::Promotion_GlobalStatus>(slot, 0);
			obj->async_save(true);
			it = g_status_map->emplace_hint(it, slot, std::move(obj));
		}
		return it->second;
	};

	const auto utc_now = Poseidon::get_utc_time();

	const auto server_created_time_obj      = get_obj(SLOT_SERVER_CREATED_TIME);
	const auto first_balancing_time_obj     = get_obj(SLOT_FIRST_BALANCING_TIME);
	const auto acc_card_unit_price_obj       = get_obj(SLOT_ACC_CARD_UNIT_PRICE);
	const auto server_daily_reset_time_obj   = get_obj(SLOT_SERVER_DAILY_RESET_TIME);
	const auto first_balancing_done_obj     = get_obj(SLOT_FIRST_BALANCING_DONE);

	if(server_created_time_obj->get_value() == 0){
		LOG_EMPERY_PROMOTION_WARNING("This seems to be the first run?");

		const auto first_balancing_time     = Poseidon::scan_time(
	                                        get_config<std::string>("first_balancing_time").c_str());
		const auto acc_card_unit_price_begin  = get_config<boost::uint64_t>("acc_card_unit_price_begin", 40000);

		auto root_user_name = get_config<std::string>("init_root_username", "root");
		auto root_nick     = get_config<std::string>("init_root_nick",     "root");
		auto root_password = get_config<std::string>("init_root_password", "root");

		LOG_EMPERY_PROMOTION_DEBUG("Determining max promotion level...");
		const auto max_promotion_data = Data::Promotion::get(UINT64_MAX);
		if(!max_promotion_data){
			LOG_EMPERY_PROMOTION_ERROR("Failed to determine max promotion level!");
			DEBUG_THROW(Exception, sslit("Failed to determine max promotion level"));
		}

		LOG_EMPERY_PROMOTION_WARNING("Creating root user: root_user_name = ", root_user_name, ", root_password = ", root_password,
			", level = ", max_promotion_data->level);
		const auto account_id = AccountMap::create(std::move(root_user_name), std::string(), std::move(root_nick),
			root_password, root_password, AccountId(0), AccountMap::FL_ROBOT, "127.0.0.1");
		LOG_EMPERY_PROMOTION_INFO("> account_id = ", account_id);

		server_created_time_obj->set_value(utc_now);
		first_balancing_time_obj->set_value(first_balancing_time);
		acc_card_unit_price_obj->set_value(acc_card_unit_price_begin);
		server_daily_reset_time_obj->set_value(utc_now);
	}

	const auto last_reset_time = server_daily_reset_time_obj->get_value();
	server_daily_reset_time_obj->set_value(utc_now);

	const auto first_balancing_time = first_balancing_time_obj->get_value();
	if(first_balancing_time < utc_now){
		if(first_balancing_done_obj->get_value() == false){
			first_balancing_done_obj->set_value(true);

			try {
				LOG_EMPERY_PROMOTION_WARNING("Commit first balance bonus!");
				commit_first_balance_bonus();
				LOG_EMPERY_PROMOTION_WARNING("Done committing first balance bonus.");
			} catch(std::exception &e){
				LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
			}
		}

		// 首次结算之前不涨价。
		const auto daily_reset_at_o_clock = get_config<unsigned>("daily_reset_at_o_clock", 16);
		const auto this_day = (utc_now - daily_reset_at_o_clock * 3600000) / 86400000;
		const auto last_day = (last_reset_time - daily_reset_at_o_clock * 3600000) / 86400000;
		if(last_day < this_day){
			const auto delta_days = this_day - last_day;
			LOG_EMPERY_PROMOTION_INFO("Daily reset: delta_days = ", delta_days);

			const auto acc_card_unit_price_increment = get_config<boost::uint64_t>("acc_card_unit_price_increment", 100);
			const auto acc_card_unit_price_begin     = get_config<boost::uint64_t>("acc_card_unit_price_begin",     40000);
			const auto acc_card_unit_price_end       = get_config<boost::uint64_t>("acc_card_unit_price_end",       50000);

			auto new_acc_card_unit_price = acc_card_unit_price_obj->get_value();
			const auto delta_price = checked_mul(acc_card_unit_price_increment, delta_days);
			LOG_EMPERY_PROMOTION_INFO("Incrementing acceleration card unit price by ", delta_price);
			new_acc_card_unit_price = checked_add(new_acc_card_unit_price, delta_price);
			if(new_acc_card_unit_price < acc_card_unit_price_begin){
				new_acc_card_unit_price = acc_card_unit_price_begin;
			}
			if(new_acc_card_unit_price > acc_card_unit_price_end){
				new_acc_card_unit_price = acc_card_unit_price_end;
			}
			LOG_EMPERY_PROMOTION_INFO("Incremented acceleration card unit price to ", new_acc_card_unit_price);
			acc_card_unit_price_obj->set_value(new_acc_card_unit_price);
		}
	}
}

}
