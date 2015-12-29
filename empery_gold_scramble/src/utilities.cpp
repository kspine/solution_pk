#include "precompiled.hpp"
#include "utilities.hpp"
#include <poseidon/async_job.hpp>
#include <poseidon/json.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "mysql/game_history.hpp"
#include "singletons/global_status.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/bid_record_map.hpp"
#include "singletons/http_client_daemon.hpp"
#include "player_session.hpp"
#include "msg/sc_account.hpp"

namespace EmperyGoldScramble {

namespace {
	std::uint64_t get_number_of_winners(){
		PROFILE_ME;

		const auto player_count = BidRecordMap::get_size();
		const auto percent_winners = GlobalStatus::get(GlobalStatus::SLOT_PERCENT_WINNERS);
		auto count = static_cast<std::uint64_t>(std::ceil(percent_winners / 100.0 * player_count - 0.001));
		if(count > player_count){
			count = player_count;
		}
		return count;
	}
	Msg::SC_AccountAuctionStatus make_auction_status_message(){
		PROFILE_ME;

		const auto begin_time             = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
		const auto end_time               = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		const auto gold_coins_in_pot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto account_balance_in_pot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);
		const auto percent_winners        = GlobalStatus::get(GlobalStatus::SLOT_PERCENT_WINNERS);

		const auto utc_now = Poseidon::get_utc_time();

		const auto visible_bid_record_count = get_config<std::size_t>("visible_bid_record_count", 100);
		std::vector<BidRecordMap::Record> records;
		BidRecordMap::get_all(records, visible_bid_record_count);

		Msg::SC_AccountAuctionStatus msg;
		msg.server_time           = utc_now;
		msg.begin_time            = begin_time;
		msg.end_duration          = (utc_now < begin_time) ? 0 : saturated_sub(end_time, utc_now);
		msg.gold_coins_in_pot      = gold_coins_in_pot;
		msg.account_balance_in_pot = account_balance_in_pot;
		msg.number_of_winners      = get_number_of_winners();
		msg.percent_winners        = percent_winners;
		msg.records.reserve(records.size());
		for(auto it = records.rbegin(); it != records.rend(); ++it){
			msg.records.emplace_back();
			auto &record = msg.records.back();
			record.record_auto_id  = it->record_auto_id;
			record.timestamp       = it->timestamp;
			record.login_name      = std::move(it->login_name);
			record.nick            = std::move(it->nick);
			record.gold_coins      = it->gold_coins;
			record.account_balance = it->account_balance;
		}
		return msg;
	}

	using SessionMap = boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>>;

	template<typename MsgT>
	void broadcast_message(const SessionMap &sessions, const MsgT &msg){
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Broadcast message: ", msg);

		const auto msg_id = static_cast<unsigned>(msg.ID);
		const auto msg_payload = Poseidon::StreamBuffer(msg);
		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = it->second;
			try {
				session->send(msg_id, msg_payload);
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
				session->force_shutdown();
			}
		}
	}

	boost::weak_ptr<Poseidon::TimerItem> g_timer;

	Msg::SC_AccountLastLog g_last_log_msg;

	void commit_game_reward(const std::string &login_name, const std::string &nick, std::uint64_t game_auto_id,
		std::uint64_t gold_coins, std::uint64_t account_balance,
		std::uint64_t game_begin_time, std::uint64_t gold_coins_in_pot, std::uint64_t account_balance_in_pot)
	{
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Commit game reward: login_name = ", login_name, ", game_auto_id = ", game_auto_id,
			", gold_coins = ", gold_coins, ", account_balance = ", account_balance, ", game_begin_time = ", game_begin_time,
			", gold_coins_in_pot = ", gold_coins_in_pot, ", account_balance_in_pot = ", account_balance_in_pot);

		auto promotion_server_host    = get_config<std::string>("promotion_http_server_host",           "127.0.0.1");
		auto promotion_server_port    = get_config<unsigned>   ("promotion_http_server_port",           6212);
		auto promotion_server_use_ssl = get_config<bool>       ("promotion_http_server_use_ssl",        0);
		auto promotion_server_auth    = get_config<std::string>("promotion_http_server_auth_user_pass", "");
		auto promotion_server_path    = get_config<std::string>("promotion_http_server_path",           "/empery_promotion/account/");

		const auto history_obj = boost::make_shared<MySql::GoldScramble_GameHistory>(
			GlobalStatus::fetch_add(GlobalStatus::SLOT_RECORD_AUTO_ID, 1), game_auto_id,
			Poseidon::get_utc_time(), login_name, nick, gold_coins, account_balance);

		Poseidon::OptionalMap params;
		params.set(sslit("loginName"), std::move(login_name));
		params.set(sslit("goldCoins"), boost::lexical_cast<std::string>(gold_coins));
		params.set(sslit("accountBalance"), boost::lexical_cast<std::string>(account_balance));
		params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(game_begin_time));
		params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(gold_coins_in_pot));
		params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(account_balance_in_pot));

		unsigned retry_count = 3;
	_retry:
		auto response = HttpClientDaemon::get(promotion_server_host, promotion_server_port, promotion_server_use_ssl, promotion_server_auth,
 			promotion_server_path + "notifyGoldScrambleReward", std::move(params));
		if(response.status_code != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: status_code = ", response.status_code);
			if(retry_count == 0){
				LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Error committing reward!");
				DEBUG_THROW(Exception, sslit("Error committing reward"));
			}
			--retry_count;
			goto _retry;
		}

		history_obj->async_save(true);

		const auto session = PlayerSessionMap::get(login_name);
		if(session){
			try {
				std::istringstream iss(response.entity.dump());
				auto root = Poseidon::JsonParser::parse_object(iss);
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
				const auto gold_coins = static_cast<std::uint64_t>(root.at(sslit("goldCoins")).get<double>());
				const auto account_balance = static_cast<std::uint64_t>(root.at(sslit("accountBalance")).get<double>());
				session->send(Msg::SC_AccountAccountBalance(gold_coins, account_balance));
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
				session->force_shutdown();
			}
		}
	}

	void check_game_end(){
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Checking game end...");

		invalidate_auction_status();

		const auto utc_now = Poseidon::get_utc_time();

		const auto game_begin_time = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
		if(utc_now < game_begin_time){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("No game in progress.");
			const auto timer = g_timer.lock();
			if(timer){
				Poseidon::TimerDaemon::set_time(timer, game_begin_time - utc_now);
			}
			return;
		}

		const auto game_end_time = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		if(utc_now < game_end_time){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Game is in progress. Check it later.");
			const auto timer = g_timer.lock();
			if(timer){
				Poseidon::TimerDaemon::set_time(timer, game_end_time - utc_now);
			}
			return;
		}

		SessionMap sessions;
		PlayerSessionMap::get_all(sessions);

		const auto gold_coins_in_pot = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto account_balance_in_pot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

		std::vector<BidRecordMap::Record> records;
		BidRecordMap::get_all(records, get_number_of_winners());
		const auto number_of_winners = records.size();
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Calculating game result: number_of_winners = ", number_of_winners);
		if(number_of_winners > 0){
			const auto game_auto_id = GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID);
			try {
				const auto gold_coins_per_person = static_cast<std::uint64_t>(gold_coins_in_pot / number_of_winners);
				const auto account_balance_per_person = static_cast<std::uint64_t>(account_balance_in_pot / 100 / number_of_winners) * 100;

				auto it = records.begin();
				while(it != records.end() - 1){
					LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("> Winner: login_name = ", it->login_name);
					try {
						Poseidon::enqueue_async_job(std::bind(&commit_game_reward, it->login_name, it->nick, game_auto_id,
							gold_coins_per_person, account_balance_per_person,
							game_begin_time, gold_coins_in_pot, account_balance_in_pot));
					} catch(std::exception &e){
						LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
					}
					++it;
				}
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("> First winner: login_name = ", it->login_name);
				try {
					Poseidon::enqueue_async_job(std::bind(&commit_game_reward, it->login_name, it->nick, game_auto_id,
						gold_coins_in_pot - gold_coins_per_person * (number_of_winners - 1),
						account_balance_in_pot - account_balance_per_person * (number_of_winners - 1),
						game_begin_time, gold_coins_in_pot, account_balance_in_pot));
				} catch(std::exception &e){
					LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
				}
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
			}
		}
		BidRecordMap::clear();
		GlobalStatus::fetch_add(GlobalStatus::SLOT_GAME_AUTO_ID, 1);
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Done calculating game result.");

		const auto game_start_o_clock    = get_config<unsigned>       ("game_start_o_clock",    2);
		const auto game_end_o_clock      = get_config<unsigned>       ("game_end_o_clock",      12);
		const auto minimum_game_duration = get_config<std::uint64_t>("minimum_game_duration", 1800000);
		const auto next_game_delay       = get_config<std::uint64_t>("next_game_delay",       300000);

		const auto init_gold_coins_in_pot      = get_config<std::uint64_t>("init_gold_coins_in_pot",      500);
		const auto init_account_balance_in_pot = get_config<std::uint64_t>("init_account_balance_in_pot", 50000);

		const auto percent_winners_low  = get_config<unsigned>("percent_winners_low",  1);
		const auto percent_winners_high = get_config<unsigned>("percent_winners_high", 5);

		auto dt = Poseidon::break_down_time(utc_now);
		std::uint64_t next_game_begin_time;
		if(dt.hr < game_start_o_clock){
			// 设为当天开始时间。
			dt.hr = game_start_o_clock;
			dt.min = 0;
			dt.sec = 0;
			dt.ms = 0;
			next_game_begin_time = Poseidon::assemble_time(dt);
		} else if(dt.hr >= game_end_o_clock){
			// 设为第二天开始时间。
			dt.hr = game_start_o_clock;
			dt.min = 0;
			dt.sec = 0;
			dt.ms = 0;
			next_game_begin_time = Poseidon::assemble_time(dt) + 86400 * 1000;
		} else {
			next_game_begin_time = utc_now + next_game_delay;
		}
		const auto next_game_end_time = next_game_begin_time + minimum_game_duration;

		const auto percent_winners = Poseidon::rand32(percent_winners_low, percent_winners_high);
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Percent of winners: percent_winners = ", percent_winners,
			", percent_winners_low = ", percent_winners_low, ", percent_winners_high = ", percent_winners_high);

		GlobalStatus::exchange(GlobalStatus::SLOT_GAME_BEGIN_TIME, next_game_begin_time);
		GlobalStatus::exchange(GlobalStatus::SLOT_GAME_END_TIME,   next_game_end_time);

		GlobalStatus::exchange(GlobalStatus::SLOT_GOLD_COINS_IN_POT,      init_gold_coins_in_pot);
		GlobalStatus::exchange(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT, init_account_balance_in_pot);

		GlobalStatus::exchange(GlobalStatus::SLOT_PERCENT_WINNERS, percent_winners);

		const auto timer = g_timer.lock();
		if(timer){
			Poseidon::TimerDaemon::set_time(timer, next_game_begin_time - utc_now);
		}

		if(!sessions.empty()){
			const auto visible_winner_count = get_config<std::size_t>("visible_winner_count", 100);

			Msg::SC_AccountLastLog msg;
			msg.game_begin_time       = game_begin_time;
			msg.gold_coins_in_pot      = gold_coins_in_pot;
			msg.account_balance_in_pot = account_balance_in_pot;
			msg.number_of_winners     = number_of_winners;

			auto rend = records.rend();
			if(records.size() > visible_winner_count){
				rend -= static_cast<std::ptrdiff_t>(records.size() - visible_winner_count);
			}
			msg.players.reserve(static_cast<std::size_t>(rend - records.rbegin()));
			for(auto it = records.rbegin(); it != rend; ++it){
				msg.players.emplace_back();
				auto &player = msg.players.back();
				player.login_name = it->login_name;
				player.nick = it->nick;
			}

			broadcast_message(sessions, msg);
			g_last_log_msg = std::move(msg);
		}
	}

	MODULE_RAII(handles){
		auto timer = Poseidon::TimerDaemon::register_timer(0, 5000, std::bind(&check_game_end));
		g_timer = timer;
		handles.push(std::move(timer));
	}
}

void send_auction_status_to_client(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	session->send(make_auction_status_message());
}
void send_last_log_to_client(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	session->send(g_last_log_msg);
}
void invalidate_auction_status(){
	PROFILE_ME;

	GlobalStatus::exchange(GlobalStatus::SLOT_STATUS_INVALIDATED, true);

	Poseidon::enqueue_async_job([]{
		PROFILE_ME;

		const bool invalidated = GlobalStatus::get(GlobalStatus::SLOT_STATUS_INVALIDATED);
		if(!invalidated){
			return;
		}
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Notifying auction status to all clients...");

		SessionMap sessions;
		PlayerSessionMap::get_all(sessions);
		if(!sessions.empty()){
			broadcast_message(sessions, make_auction_status_message());
		}

		GlobalStatus::exchange(GlobalStatus::SLOT_STATUS_INVALIDATED, false);
	});
}

}
