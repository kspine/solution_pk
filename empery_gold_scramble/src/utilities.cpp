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
#include "checked_arithmetic.hpp"

namespace EmperyGoldScramble {

namespace {
	boost::uint64_t getNumberOfWinners(){
		PROFILE_ME;

		const auto playerCount = BidRecordMap::getSize();
		const auto percentWinners = GlobalStatus::get(GlobalStatus::SLOT_PERCENT_WINNERS);
		auto count = static_cast<boost::uint64_t>(std::ceil(percentWinners / 100.0 * playerCount - 0.001));
		if(count > playerCount){
			count = playerCount;
		}
		return count;
	}
	Msg::SC_AccountAuctionStatus makeAuctionStatusMessage(){
		PROFILE_ME;

		const auto beginTime           = GlobalStatus::get(GlobalStatus::SLOT_GAME_BEGIN_TIME);
		const auto endTime             = GlobalStatus::get(GlobalStatus::SLOT_GAME_END_TIME);
		const auto goldCoinsInPot      = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);
		const auto percentWinners      = GlobalStatus::get(GlobalStatus::SLOT_PERCENT_WINNERS);

		const auto utcNow = Poseidon::getUtcTime();

		const auto visibleBidRecordCount = getConfig<std::size_t>("visible_bid_record_count", 100);
		std::vector<BidRecordMap::Record> records;
		BidRecordMap::getAll(records, visibleBidRecordCount);

		Msg::SC_AccountAuctionStatus msg;
		msg.serverTime          = utcNow;
		msg.beginTime           = beginTime;
		msg.endDuration         = (utcNow < beginTime) ? 0 : saturatedSub(endTime, utcNow);
		msg.goldCoinsInPot      = goldCoinsInPot;
		msg.accountBalanceInPot = accountBalanceInPot;
		msg.numberOfWinners     = getNumberOfWinners();
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
		return msg;
	}

	using SessionMap = boost::container::flat_map<std::string, boost::shared_ptr<PlayerSession>>;

	template<typename MsgT>
	void broadcastMessage(const SessionMap &sessions, const MsgT &msg){
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Broadcast message: ", msg);

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
	}

	boost::weak_ptr<Poseidon::TimerItem> g_timer;

	Msg::SC_AccountLastLog g_lastLogMsg;

	void commitGameReward(const std::string &loginName, const std::string &nick, boost::uint64_t gameAutoId,
		boost::uint64_t goldCoins, boost::uint64_t accountBalance,
		boost::uint64_t gameBeginTime, boost::uint64_t goldCoinsInPot, boost::uint64_t accountBalanceInPot)
	{
		PROFILE_ME;
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Commit game reward: loginName = ", loginName, ", gameAutoId = ", gameAutoId,
			", goldCoins = ", goldCoins, ", accountBalance = ", accountBalance, ", gameBeginTime = ", gameBeginTime,
			", goldCoinsInPot = ", goldCoinsInPot, ", accountBalanceInPot = ", accountBalanceInPot);

		auto promotionServerHost   = getConfig<std::string>("promotion_http_server_host",           "127.0.0.1");
		auto promotionServerPort   = getConfig<unsigned>   ("promotion_http_server_port",           6212);
		auto promotionServerUseSsl = getConfig<bool>       ("promotion_http_server_use_ssl",        0);
		auto promotionServerAuth   = getConfig<std::string>("promotion_http_server_auth_user_pass", "");
		auto promotionServerPath   = getConfig<std::string>("promotion_http_server_path",           "/empery_promotion/account/");

		const auto historyObj = boost::make_shared<MySql::GoldScramble_GameHistory>(
			GlobalStatus::fetchAdd(GlobalStatus::SLOT_RECORD_AUTO_ID, 1), gameAutoId,
			Poseidon::getUtcTime(), loginName, nick, goldCoins, accountBalance);

		Poseidon::OptionalMap params;
		params.set(sslit("loginName"), std::move(loginName));
		params.set(sslit("goldCoins"), boost::lexical_cast<std::string>(goldCoins));
		params.set(sslit("accountBalance"), boost::lexical_cast<std::string>(accountBalance));
		params.set(sslit("gameBeginTime"), boost::lexical_cast<std::string>(gameBeginTime));
		params.set(sslit("goldCoinsInPot"), boost::lexical_cast<std::string>(goldCoinsInPot));
		params.set(sslit("accountBalanceInPot"), boost::lexical_cast<std::string>(accountBalanceInPot));

		unsigned retryCount = 3;
	_retry:
		auto response = HttpClientDaemon::get(promotionServerHost, promotionServerPort, promotionServerUseSsl, promotionServerAuth,
 			promotionServerPath + "notifyGoldScrambleReward", std::move(params));
		if(response.statusCode != Poseidon::Http::ST_OK){
			LOG_EMPERY_GOLD_SCRAMBLE_WARNING("Promotion server returned an HTTP error: statusCode = ", response.statusCode);
			if(retryCount == 0){
				LOG_EMPERY_GOLD_SCRAMBLE_ERROR("Error committing reward!");
				DEBUG_THROW(Exception, sslit("Error committing reward"));
			}
			--retryCount;
			goto _retry;
		}

		historyObj->asyncSave(true);

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

		invalidateAuctionStatus();

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

		SessionMap sessions;
		PlayerSessionMap::getAll(sessions);

		const auto goldCoinsInPot = GlobalStatus::get(GlobalStatus::SLOT_GOLD_COINS_IN_POT);
		const auto accountBalanceInPot = GlobalStatus::get(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT);

		std::vector<BidRecordMap::Record> records;
		BidRecordMap::getAll(records, getNumberOfWinners());
		const auto numberOfWinners = records.size();
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Calculating game result: numberOfWinners = ", numberOfWinners);
		if(numberOfWinners > 0){
			const auto gameAutoId = GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID);
			try {
				const auto goldCoinsPerPerson = static_cast<boost::uint64_t>(goldCoinsInPot / numberOfWinners);
				const auto accountBalancePerPerson = static_cast<boost::uint64_t>(accountBalanceInPot / 100 / numberOfWinners) * 100;

				auto it = records.begin();
				while(it != records.end() - 1){
					LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("> Winner: loginName = ", it->loginName);
					try {
						Poseidon::enqueueAsyncJob(std::bind(&commitGameReward, it->loginName, it->nick, gameAutoId,
							goldCoinsPerPerson, accountBalancePerPerson,
							gameBeginTime, goldCoinsInPot, accountBalanceInPot));
					} catch(std::exception &e){
						LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
					}
					++it;
				}
				LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("> First winner: loginName = ", it->loginName);
				try {
					Poseidon::enqueueAsyncJob(std::bind(&commitGameReward, it->loginName, it->nick, gameAutoId,
						goldCoinsInPot - goldCoinsPerPerson * (numberOfWinners - 1),
						accountBalanceInPot - accountBalancePerPerson * (numberOfWinners - 1),
						gameBeginTime, goldCoinsInPot, accountBalanceInPot));
				} catch(std::exception &e){
					LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
				}
			} catch(std::exception &e){
				LOG_EMPERY_GOLD_SCRAMBLE_ERROR("std::exception thrown: what = ", e.what());
			}
		}
		BidRecordMap::clear();
		GlobalStatus::fetchAdd(GlobalStatus::SLOT_GAME_AUTO_ID, 1);
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Done calculating game result.");

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
		const auto nextGameEndTime = nextGameBeginTime + minimumGameDuration;

		const auto percentWinners = Poseidon::rand32(percentWinnersLow, percentWinnersHigh);

		GlobalStatus::exchange(GlobalStatus::SLOT_GAME_BEGIN_TIME, nextGameBeginTime);
		GlobalStatus::exchange(GlobalStatus::SLOT_GAME_END_TIME,   nextGameEndTime);

		GlobalStatus::exchange(GlobalStatus::SLOT_GOLD_COINS_IN_POT,      initGoldCoinsInPot);
		GlobalStatus::exchange(GlobalStatus::SLOT_ACCOUNT_BALANCE_IN_POT, initAccountBalanceInPot);

		GlobalStatus::exchange(GlobalStatus::SLOT_PERCENT_WINNERS, percentWinners);

		const auto timer = g_timer.lock();
		if(timer){
			Poseidon::TimerDaemon::setTime(timer, nextGameBeginTime - utcNow);
		}

		if(!sessions.empty()){
			const auto visibleWinnerCount = getConfig<std::size_t>("visible_winner_count", 100);

			Msg::SC_AccountLastLog msg;
			msg.gameBeginTime       = gameBeginTime;
			msg.goldCoinsInPot      = goldCoinsInPot;
			msg.accountBalanceInPot = accountBalanceInPot;
			msg.numberOfWinners     = numberOfWinners;

			auto rend = records.rend();
			if(records.size() > visibleWinnerCount){
				rend -= static_cast<std::ptrdiff_t>(records.size() - visibleWinnerCount);
			}
			msg.players.reserve(static_cast<std::size_t>(rend - records.rbegin()));
			for(auto it = records.rbegin(); it != rend; ++it){
				msg.players.emplace_back();
				auto &player = msg.players.back();
				player.loginName = it->loginName;
				player.nick = it->nick;
			}

			broadcastMessage(sessions, msg);
			g_lastLogMsg = std::move(msg);
		}
	}

	MODULE_RAII(handles){
		auto timer = Poseidon::TimerDaemon::registerTimer(0, 5000, std::bind(&checkGameEnd));
		g_timer = timer;
		handles.push(std::move(timer));
	}
}

void sendAuctionStatusToClient(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	session->send(makeAuctionStatusMessage());
}
void sendLastLogToClient(const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	session->send(g_lastLogMsg);
}
void invalidateAuctionStatus(){
	PROFILE_ME;

	GlobalStatus::exchange(GlobalStatus::SLOT_STATUS_INVALIDATED, true);

	Poseidon::enqueueAsyncJob([]{
		PROFILE_ME;

		const bool invalidated = GlobalStatus::get(GlobalStatus::SLOT_STATUS_INVALIDATED);
		if(!invalidated){
			return;
		}
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Notifying auction status to all clients...");

		SessionMap sessions;
		PlayerSessionMap::getAll(sessions);
		if(!sessions.empty()){
			broadcastMessage(sessions, makeAuctionStatusMessage());
		}

		GlobalStatus::exchange(GlobalStatus::SLOT_STATUS_INVALIDATED, false);
	});
}

}
