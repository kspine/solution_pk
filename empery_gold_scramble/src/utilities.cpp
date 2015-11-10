#include "precompiled.hpp"
#include "utilities.hpp"
#include <poseidon/async_job.hpp>
#include <poseidon/json.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "singletons/global_status.hpp"
#include "singletons/player_session_map.hpp"
#include "singletons/bid_record_map.hpp"
#include "singletons/http_client_daemon.hpp"
#include "player_session.hpp"
#include "msg/sc_account.hpp"
#include "checked_arithmetic.hpp"

namespace EmperyGoldScramble {

namespace {
	boost::weak_ptr<Poseidon::TimerItem> g_timer;

	void commitGameReward(const std::string &loginName, boost::uint64_t goldCoins, boost::uint64_t accountBalance,
		boost::uint64_t gameBeginTime, boost::uint64_t goldCoinsInPot, boost::uint64_t accountBalanceInPot)
	{
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Commit game reward: loginName = ", loginName,
			", goldCoins = ", goldCoins, ", accountBalance = ", accountBalance, ", gameBeginTime = ", gameBeginTime,
			", goldCoinsInPot = ", goldCoinsInPot, ", accountBalanceInPot = ", accountBalanceInPot);

		auto promotionServerHost   = getConfig<std::string>("promotion_http_server_host",           "127.0.0.1");
		auto promotionServerPort   = getConfig<unsigned>   ("promotion_http_server_port",           6212);
		auto promotionServerUseSsl = getConfig<bool>       ("promotion_http_server_use_ssl",        0);
		auto promotionServerAuth   = getConfig<std::string>("promotion_http_server_auth_user_pass", "");
		auto promotionServerPath   = getConfig<std::string>("promotion_http_server_path",           "/empery_promotion/account/");

		Poseidon::OptionalMap params;
		params.set(sslit("loginName"), std::move(loginName));
		params.set(sslit("goldCoins"), boost::lexical_cast<std::string>(goldCoins));
		params.set(sslit("accountBalance"), boost::lexical_cast<std::string>(accountBalance));
		params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(gameBeginTime));
		params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(goldCoinsInPot));
		params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(accountBalanceInPot));

		unsigned retry = 3;
	_retry:
		auto response = HttpClientDaemon::get(promotionServerHost, promotionServerPort, promotionServerUseSsl, promotionServerAuth,
 			promotionServerPath + "notifyGoldScrambleReward", std::move(params));
		if(response.statusCode != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: statusCode = ", response.statusCode);
			if(retry == 0){
				LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Error committing reward!");
				DEBUG_THROW(Exception, sslit("Error committing reward"));
			}
			--retry;
			goto _retry;
		}

		const auto session = PlayerSessionMap::get(loginName);
		if(session){
			try {
				std::istringstream iss(response.entity.dump());
				auto root = Poseidon::JsonParser::parseObject(iss);
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Promotion server response: ", root.dump());
				const auto goldCoins = static_cast<boost::uint64_t>(root.at(sslit("goldCoins")).get<double>());
				const auto accountBalance = static_cast<boost::uint64_t>(root.at(sslit("accountBalance")).get<double>());
				session->send(Msg::SC_AccountAccountBalance(goldCoins, accountBalance));
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
				session->forceShutdown();
			}
		}
	}

	void checkGameEnd(){
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Checking game end...");

		const auto utcNow = Poseidon::getUtcTime();

		const auto gameBeginTime = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
		if(utcNow < gameBeginTime){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("No game in progress.");
			const auto timer = g_timer.lock();
			if(timer){
				Poseidon::TimerDaemon::setTime(timer, gameBeginTime - utcNow);
			}
			return;
		}

		const auto gameEndTime = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		if(utcNow < gameEndTime){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Game is in progress. Check it later.");
			const auto timer = g_timer.lock();
			if(timer){
				Poseidon::TimerDaemon::setTime(timer, gameEndTime - utcNow);
			}
			return;
		}

		const auto goldCoinsInPot = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Calculating game result!");
		try {
			const auto playerCount = BidRecordMap::getSize();
			const auto percentWinners = GlobalStatus::get(GlobalStatus::SLOT_PERCENT_WINNERS);
			const auto numberOfWinners = static_cast<boost::uint64_t>(percentWinners / 100.0 * playerCount);
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("> playerCount = ", playerCount,
				", percentWinners = ", percentWinners, ", numberOfWinners = ", numberOfWinners);
			if(numberOfWinners > 0){
				std::vector<BidRecordMap::Record> records;
				BidRecordMap::getAll(records, numberOfWinners);
				const auto goldCoinsPerPerson = static_cast<boost::uint64_t>(goldCoinsInPot / numberOfWinners);
				const auto accountBalancePerPerson = static_cast<boost::uint64_t>(accountBalanceInPot / 100 / numberOfWinners) * 100;
				for(auto it = records.begin(); it != records.end() - 1; ++it){
					try {
						Poseidon::enqueueAsyncJob(std::bind(&commitGameReward, std::move(it->loginName),
							goldCoinsPerPerson, accountBalancePerPerson,
							gameBeginTime, goldCoinsInPot, accountBalanceInPot));
					} catch(std::exception &e){
						LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
					}
				}
				try {
					Poseidon::enqueueAsyncJob(std::bind(&commitGameReward, std::move(records.back().loginName),
						goldCoinsInPot - goldCoinsPerPerson * numberOfWinners, accountBalanceInPot - accountBalancePerPerson * numberOfWinners,
						gameBeginTime, goldCoinsInPot, accountBalanceInPot));
				} catch(std::exception &e){
					LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
				}
			}
		} catch(std::exception &e){
			LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
		}
		BidRecordMap::clear();
		GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_AUTO_ID, 1);

		const auto gameStartOClock     = getConfig<unsigned>       ("game_start_o_clock",    2);
		const auto gameEndOClock       = getConfig<unsigned>       ("game_end_o_clock",      12);
		const auto minimumGameDuration = getConfig<boost::uint64_t>("minimum_game_duration", 1800000);
		const auto nextGameDelay       = getConfig<boost::uint64_t>("next_game_delay",       300000);

		const auto initGoldCoinsInPot      = getConfig<boost::uint64_t>("init_gold_coins_in_pot",      500);
		const auto initAccountBalanceInPot = getConfig<boost::uint64_t>("init_account_balance_in_pot", 50000);

		const auto percentWinnersLow  = getConfig<unsigned>("percent_winners_low",  1);
		const auto percentWinnersHigh = getConfig<unsigned>("percent_winners_high", 5);

		auto dt = Poseidon::breakDownTime(utcNow);
		boost::uint64_t nextGameBeginTime;
		if(dt.hr < gameStartOClock){
			// 设为当天开始时间。
			dt.hr = gameStartOClock;
			nextGameBeginTime = Poseidon::assembleTime(dt);
		} else if(dt.hr > gameEndOClock){
			// 设为第二天开始时间。
			dt.hr = gameStartOClock;
			nextGameBeginTime = Poseidon::assembleTime(dt) + 86400 * 1000;
		} else {
			nextGameBeginTime = utcNow + nextGameDelay;
		}

		const auto percentWinners = Poseidon::rand32(percentWinnersLow, percentWinnersHigh);

		GlobalStatus::exchange(GlobalStatus::SLOT_GAME_BEGIN_TIME, nextGameBeginTime);
		GlobalStatus::exchange(GlobalStatus::SLOT_GAME_END_TIME,   nextGameBeginTime + minimumGameDuration);

		GlobalStatus::exchange(GlobalStatus::SLOT_GOLD_COINS_IN_POT,      initGoldCoinsInPot);
		GlobalStatus::exchange(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT, initAccountBalanceInPot);

		GlobalStatus::exchange(GlobalStatus::SLOT_PERCENT_WINNERS, percentWinners);

		updateAuctionStatus();
	}

	MODULE_RAII(handles){
		auto timer = Poseidon::TimerDaemon::registerTimer(0, 1000, std::bind(&checkGameEnd));
		g_timer = timer;
		handles.push(std::move(timer));
	}
}

void updateAuctionStatus(){
	PROFILE_ME;

	const auto utcNow = Poseidon::getUtcTime();
	const auto gameEndTime = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
	if(saturatedSub(gameEndTime, utcNow) <= 30000){
		const auto goldCoinsInPot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);
		const auto sum = saturatedAdd(goldCoinsInPot, accountBalanceInPot / 100);
		if(sum > 10000){
			GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_END_TIME, 1000);
		} else if(sum > 5000){
			GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_END_TIME, 2000);
		} else {
			GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_END_TIME, 10000);
		}
	}

	GlobalStatus::exchange(GlobalStatus::SLOT_STATUS_INVALIDATED, true);

	Poseidon::enqueueAsyncJob([]{
		PROFILE_ME;

		const bool invalidated = GlobalStatus::get(GlobalStatus::SLOT_STATUS_INVALIDATED);
		if(!invalidated){
			return;
		}
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Notifying auction status to all clients...");

		boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>> sessions;
		PlayerSessionMap::getAll(sessions);
		if(sessions.empty()){
			LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("No player is online.");
			return;
		}

		const auto beginTime           = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
		const auto endTime             = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		const auto goldCoinsInPot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);
		const auto percentWinners      = GlobalStatus::get(GlobalStatus::SLOT_PERCENT_WINNERS);

		const auto utcNow = Poseidon::getUtcTime();
		const auto playerCount = BidRecordMap::getSize();

		const auto visibleBidRecordCount = getConfig<std::size_t>("visible_bid_record_count", 100);
		std::vector<BidRecordMap::Record> records;
		BidRecordMap::getAll(records, visibleBidRecordCount);

		Msg::SC_AccountAuctionStatus msg;
		msg.serverTime          = utcNow;
		msg.beginTime           = beginTime;
		msg.endDuration         = saturatedSub(endTime, utcNow);
		msg.goldCoinsInPot      = goldCoinsInPot;
		msg.accountBalanceInPot = accountBalanceInPot;
		msg.numberOfWinners     = static_cast<boost::uint64_t>(percentWinners / 100.0 * playerCount);
		msg.percentWinners      = percentWinners;
		msg.records.reserve(records.size());
		for(auto it = records.rbegin(); it != records.rend(); ++it){
			msg.records.emplace_back();
			auto &record = msg.records.back();
			record.recordAutoId   = it->recordAutoId;
			record.timestamp      = it->timestamp;
			record.loginName      = std::move(it->loginName);
			record.nick           = std::move(it->nick);
			record.goldCoins      = it->goldCoins;
			record.accountBalance = it->accountBalance;
		}
		const auto msgId = static_cast<unsigned>(msg.ID);
		const auto msgPayload = Poseidon::StreamBuffer(msg);
		for(auto it = sessions.begin(); it != sessions.end(); ++it){
			const auto &session = it->second;
			try {
				session->send(msgId, msgPayload);
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_WARNING("std::exception thrown: what = ", e.what());
				session->forceShutdown();
			}
		}

		GlobalStatus::exchange(GlobalStatus::SLOT_STATUS_INVALIDATED, false);
	});
}

}
