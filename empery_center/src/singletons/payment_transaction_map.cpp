#include "../precompiled.hpp"
#include "payment_transaction_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../payment_transaction.hpp"
#include "../mysql/payment_transaction.hpp"
#include "../string_utilities.hpp"

namespace EmperyCenter {

namespace {
	struct PaymentTransactionElement {
		boost::shared_ptr<PaymentTransaction> payment_transaction;

		std::size_t serial_hash;
		std::uint64_t expiry_time;

		explicit PaymentTransactionElement(boost::shared_ptr<PaymentTransaction> payment_transaction_)
			: payment_transaction(std::move(payment_transaction_))
			, serial_hash(hash_string_nocase(payment_transaction->get_serial()))
			, expiry_time(payment_transaction->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(PaymentTransactionMapContainer, PaymentTransactionElement,
		MULTI_MEMBER_INDEX(serial_hash)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<PaymentTransactionMapContainer> g_payment_transaction_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Payment transaction gc timer: now = ", now);

		const auto utc_now = Poseidon::get_utc_time();

		const auto payment_transaction_map = g_payment_transaction_map.lock();
		if(payment_transaction_map){
			for(;;){
				const auto it = payment_transaction_map->begin<1>();
				if(it == payment_transaction_map->end<1>()){
					break;
				}
				if(utc_now < it->expiry_time){
					break;
				}

				LOG_EMPERY_CENTER_INFO("Reclaiming payment transaction: serial = ", it->payment_transaction->get_serial());
				payment_transaction_map->erase<1>(it);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto payment_transaction_map = boost::make_shared<PaymentTransactionMapContainer>();
		LOG_EMPERY_CENTER_INFO("Loading payment transactions...");
		std::ostringstream oss;
		const auto utc_now = Poseidon::get_utc_time();
		oss <<"SELECT * FROM `Center_PaymentTransaction` WHERE `expiry_time` > " <<Poseidon::MySql::DateFormatter(utc_now)
		    <<"  AND `settled` = FALSE AND `cancelled` = FALSE ";
		conn->execute_sql(oss.str());
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_PaymentTransaction>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto payment_transaction = boost::make_shared<PaymentTransaction>(std::move(obj));
			if(payment_transaction->is_virtually_removed()){
				continue;
			}
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
	auto acit = range.second;
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->payment_transaction->get_serial(), serial)){
			acit = it;
			break;
		}
	}
	if(acit == range.second){
		LOG_EMPERY_CENTER_WARNING("Payment transaction not found: serial = ", serial);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Payment transaction not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating payment transaction: serial = ", serial);
	if(payment_transaction->is_virtually_removed()){
		payment_transaction_map->erase<0>(acit);
	} else {
		payment_transaction_map->replace<0>(acit, PaymentTransactionElement(payment_transaction));
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
	auto acit = range.second;
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->payment_transaction->get_serial(), serial)){
			acit = it;
			break;
		}
	}
	if(acit == range.second){
		LOG_EMPERY_CENTER_WARNING("Payment transaction not found: serial = ", serial);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Removing payment transaction: serial = ", serial);
	payment_transaction_map->erase<0>(acit);
}

}
