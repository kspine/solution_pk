#include "../precompiled.hpp"
#include "payment_transaction_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../payment_transaction.hpp"
#include "../mysql/payment.hpp"
#include "../string_utilities.hpp"

namespace EmperyCenter {

namespace {
	struct PaymentTransactionElement {
		boost::shared_ptr<PaymentTransaction> payment_transaction;

		std::size_t serial_hash;
		std::pair<AccountUuid, std::uint64_t> account_uuid_created_time;
		std::uint64_t expiry_time;

		explicit PaymentTransactionElement(boost::shared_ptr<PaymentTransaction> payment_transaction_)
			: payment_transaction(std::move(payment_transaction_))
			, serial_hash(hash_string_nocase(payment_transaction->get_serial()))
			, account_uuid_created_time(payment_transaction->get_account_uuid(), payment_transaction->get_created_time())
			, expiry_time(payment_transaction->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(PaymentTransactionContainer, PaymentTransactionElement,
		MULTI_MEMBER_INDEX(serial_hash)
		MULTI_MEMBER_INDEX(account_uuid_created_time)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<PaymentTransactionContainer> g_payment_transaction_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Payment transaction gc timer: now = ", now);

		const auto payment_transaction_map = g_payment_transaction_map.lock();
		if(!payment_transaction_map){
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();
		const auto range = std::make_pair(payment_transaction_map->begin<2>(), payment_transaction_map->upper_bound<2>(utc_now));

		std::vector<boost::shared_ptr<PaymentTransaction>> payment_transactions_to_delete;
		payment_transactions_to_delete.reserve(static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			payment_transactions_to_delete.emplace_back(it->payment_transaction);
		}
		for(auto it = payment_transactions_to_delete.begin(); it != payment_transactions_to_delete.end(); ++it){
			const auto &payment_transaction = *it;
			LOG_EMPERY_CENTER_DEBUG("Reclaiming payment transaction: serial = ", payment_transaction->get_serial());
			try {
				if(!payment_transaction->has_been_committed() && !payment_transaction->has_been_cancelled()){
					payment_transaction->cancel("Payment transaction expired");
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto payment_transaction_map = boost::make_shared<PaymentTransactionContainer>();
		LOG_EMPERY_CENTER_INFO("Loading payment transactions...");
		const auto utc_now = Poseidon::get_utc_time();
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_PaymentTransaction` WHERE `expiry_time` > " <<Poseidon::MySql::DateTimeFormatter(utc_now)
		    <<"  AND `cancelled` = 0";
		conn->execute_sql(oss.str());
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_PaymentTransaction>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto payment_transaction = boost::make_shared<PaymentTransaction>(std::move(obj));
			payment_transaction_map->insert(PaymentTransactionElement(std::move(payment_transaction)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", payment_transaction_map->size(), " payment transaction(s).");
		g_payment_transaction_map = payment_transaction_map;
		handles.push(payment_transaction_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<PaymentTransaction> PaymentTransactionMap::get(const std::string &serial){
	PROFILE_ME;

	const auto payment_transaction_map = g_payment_transaction_map.lock();
	if(!payment_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Payment transaction map not loaded.");
		return { };
	}

	const auto range = payment_transaction_map->equal_range<0>(hash_string_nocase(serial));
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->payment_transaction->get_serial(), serial)){
			return it->payment_transaction;
		}
	}
	LOG_EMPERY_CENTER_TRACE("Payment transaction not found: serial = ", serial);
	return { };
}
boost::shared_ptr<PaymentTransaction> PaymentTransactionMap::require(const std::string &serial){
	PROFILE_ME;

	auto ret = get(serial);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Payment transaction not found"));
	}
	return ret;
}

void PaymentTransactionMap::get_all(std::vector<boost::shared_ptr<PaymentTransaction>> &ret){
	PROFILE_ME;

	const auto payment_transaction_map = g_payment_transaction_map.lock();
	if(!payment_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Payment transaction map not loaded.");
		return;
	}

	ret.reserve(ret.size() + payment_transaction_map->size());
	for(auto it = payment_transaction_map->begin(); it != payment_transaction_map->end(); ++it){
		ret.emplace_back(it->payment_transaction);
	}
}
void PaymentTransactionMap::get_by_account(std::vector<boost::shared_ptr<PaymentTransaction>> &ret, AccountUuid account_uuid){
	PROFILE_ME;

	const auto payment_transaction_map = g_payment_transaction_map.lock();
	if(!payment_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Payment transaction map not loaded.");
		return;
	}

	const auto lower = payment_transaction_map->lower_bound<1>(std::make_pair(account_uuid, (std::uint64_t)0));
	const auto upper = payment_transaction_map->upper_bound<1>(std::make_pair(account_uuid, (std::uint64_t)-1));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(lower, upper)));
	for(auto it = lower; it != upper; ++it){
		ret.emplace_back(it->payment_transaction);
	}
}

void PaymentTransactionMap::insert(const boost::shared_ptr<PaymentTransaction> &payment_transaction){
	PROFILE_ME;

	const auto payment_transaction_map = g_payment_transaction_map.lock();
	if(!payment_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Payment transaction map not loaded.");
		DEBUG_THROW(Exception, sslit("Payment transaction map not loaded"));
	}

	const auto &serial = payment_transaction->get_serial();

	LOG_EMPERY_CENTER_DEBUG("Inserting payment transaction: serial = ", serial);
	const auto result = payment_transaction_map->insert(PaymentTransactionElement(payment_transaction));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Payment transaction already exists: serial = ", serial);
		DEBUG_THROW(Exception, sslit("Payment transaction already exists"));
	}
}
void PaymentTransactionMap::update(const boost::shared_ptr<PaymentTransaction> &payment_transaction, bool throws_if_not_exists){
	PROFILE_ME;

	const auto payment_transaction_map = g_payment_transaction_map.lock();
	if(!payment_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Payment transaction map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Payment transaction map not loaded"));
		}
		return;
	}

	const auto &serial = payment_transaction->get_serial();

	const auto range = payment_transaction_map->equal_range<0>(hash_string_nocase(serial));
	auto it = range.second;
	for(auto tit = range.first; tit != range.second; ++tit){
		if(are_strings_equal_nocase(tit->payment_transaction->get_serial(), serial)){
			it = tit;
			break;
		}
	}
	if(it == range.second){
		LOG_EMPERY_CENTER_WARNING("Payment transaction not found: serial = ", serial);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Payment transaction not found"));
		}
		return;
	}
	if(it->payment_transaction != payment_transaction){
		LOG_EMPERY_CENTER_DEBUG("Payment transaction expired: serial = ", serial);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating payment transaction: serial = ", serial);
	const auto utc_now = Poseidon::get_utc_time();
	if(payment_transaction->get_expiry_time() < utc_now){
		payment_transaction_map->erase<0>(it);
	} else {
		payment_transaction_map->replace<0>(it, PaymentTransactionElement(payment_transaction));
	}
}
void PaymentTransactionMap::remove(const std::string &serial) noexcept {
	PROFILE_ME;

	const auto payment_transaction_map = g_payment_transaction_map.lock();
	if(!payment_transaction_map){
		LOG_EMPERY_CENTER_WARNING("Payment transaction map not loaded.");
		return;
	}

	const auto range = payment_transaction_map->equal_range<0>(hash_string_nocase(serial));
	auto it = range.second;
	for(auto tit = range.first; tit != range.second; ++tit){
		if(are_strings_equal_nocase(tit->payment_transaction->get_serial(), serial)){
			it = tit;
			break;
		}
	}
	if(it == range.second){
		LOG_EMPERY_CENTER_WARNING("Payment transaction not found: serial = ", serial);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Removing payment transaction: serial = ", serial);
	payment_transaction_map->erase<0>(it);
}

}
