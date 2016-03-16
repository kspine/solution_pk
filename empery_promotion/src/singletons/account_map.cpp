#include "../precompiled.hpp"
#include "account_map.hpp"
#include "../mmain.hpp"
#include <string.h>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include <boost/container/flat_map.hpp>
#include "../mysql/account.hpp"
#include "../mysql/account_attribute.hpp"
#include "../events/account.hpp"
#include "../data/promotion.hpp"
#include "../singletons/global_status.hpp"

namespace EmperyPromotion {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

	struct AccountElement {
		boost::shared_ptr<MySql::Promotion_Account> obj;

		AccountId account_id;
		std::string login_name;
		std::string phone_number;
		std::string nick;
		AccountId referrer_id;
		std::uint64_t level;

		explicit AccountElement(boost::shared_ptr<MySql::Promotion_Account> obj_)
			: obj(std::move(obj_))
			, account_id(obj->get_account_id()), login_name(obj->unlocked_get_login_name())
			, phone_number(obj->unlocked_get_phone_number()), nick(obj->unlocked_get_nick())
			, referrer_id(obj->get_referrer_id()), level(obj->get_level())
		{
		}
	};

	MULTI_INDEX_MAP(AccountContainer, AccountElement,
		UNIQUE_MEMBER_INDEX(account_id)
		MULTI_MEMBER_INDEX(login_name, StringCaseComparator)
		MULTI_MEMBER_INDEX(phone_number)
		MULTI_MEMBER_INDEX(nick, StringCaseComparator)
		MULTI_MEMBER_INDEX(referrer_id)
		MULTI_MEMBER_INDEX(level)
	)

	boost::shared_ptr<AccountContainer> g_account_map;

	struct AccountAttributeElement {
		boost::shared_ptr<MySql::Promotion_AccountAttribute> obj;

		AccountId account_id;
		std::pair<AccountId, unsigned> account_slot;

		explicit AccountAttributeElement(boost::shared_ptr<MySql::Promotion_AccountAttribute> obj_)
			: obj(std::move(obj_))
			, account_id(obj->get_account_id())
			, account_slot(std::make_pair(account_id, obj->get_slot()))
		{
		}
	};

	MULTI_INDEX_MAP(AccountAttributeContainer, AccountAttributeElement,
		MULTI_MEMBER_INDEX(account_id)
		UNIQUE_MEMBER_INDEX(account_slot)
	)

	boost::shared_ptr<AccountAttributeContainer> g_attribute_map;

	struct SubordinateInfoCacheElement {
		std::uint64_t max_level;
		std::uint64_t subordinate_count;
	};

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto account_map = boost::make_shared<AccountContainer>();
		LOG_EMPERY_PROMOTION_INFO("Loading accounts...");
		conn->execute_sql("SELECT * FROM `Promotion_Account`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Promotion_Account>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			account_map->insert(AccountElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", account_map->size(), " account(s).");
		g_account_map = account_map;
		handles.push(account_map);

		const auto attribute_map = boost::make_shared<AccountAttributeContainer>();
		LOG_EMPERY_PROMOTION_INFO("Loading account attributes...");
		conn->execute_sql("SELECT * FROM `Promotion_AccountAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Promotion_AccountAttribute>();
			obj->fetch(conn);
			const auto it = g_account_map->find<0>(AccountId(obj->get_account_id()));
			if(it == g_account_map->end<0>()){
				LOG_EMPERY_PROMOTION_ERROR("No such account: account_id = ", AccountId(obj->get_account_id()));
				continue;
			}
			obj->enable_auto_saving();
			attribute_map->insert(AccountAttributeElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", attribute_map->size(), " account attribute(s).");
		g_attribute_map = attribute_map;
		handles.push(attribute_map);

		const auto utc_now = Poseidon::get_utc_time();
		const auto first_balancing_time = GlobalStatus::get(GlobalStatus::SLOT_FIRST_BALANCING_TIME);
		LOG_EMPERY_PROMOTION_INFO("Updating subordinate info cache: first_balancing_time = ", first_balancing_time);
		boost::container::flat_map<AccountId, SubordinateInfoCacheElement> temp_map;
		temp_map.reserve(account_map->size());
		for(auto it = account_map->begin(); it != account_map->end(); ++it){
			auto level = it->obj->get_level();
			if((level == 0) && (utc_now >= first_balancing_time)){
				const auto promotion_data = Data::Promotion::get_first();
				if(!promotion_data){
					LOG_EMPERY_PROMOTION_FATAL("No first promotion level?");
					std::abort();
				}
				level = promotion_data->level;
			}
			auto &elem = temp_map[it->account_id];
			if(elem.max_level < level){
				elem.max_level = level;
			}

			auto referrer_it = account_map->find<0>(it->referrer_id);
			while(referrer_it != account_map->end<0>()){
				const auto current_referrer_id = referrer_it->account_id;
				auto &elem = temp_map[current_referrer_id];
				if(elem.max_level < level){
					elem.max_level = level;
				}
				++elem.subordinate_count;
				referrer_it = account_map->find<0>(referrer_it->referrer_id);
			}
		}
		for(auto it = temp_map.begin(); it != temp_map.end(); ++it){
			const auto account_it = account_map->find<0>(it->first);
			if(account_it == account_map->end<0>()){
				LOG_EMPERY_PROMOTION_WARNING("No such account: account_id = ", it->first);
				continue;
			}
			if(account_it->obj->get_max_level() != it->second.max_level){
				account_it->obj->set_max_level(it->second.max_level);
			}
			if(account_it->obj->get_subordinate_count() != it->second.subordinate_count){
				account_it->obj->set_subordinate_count(it->second.subordinate_count);
			}
		}
	}

	void fill_account_info(AccountMap::AccountInfo &info, const boost::shared_ptr<MySql::Promotion_Account> &obj){
		PROFILE_ME;

		info.account_id         = AccountId(obj->get_account_id());
		info.login_name         = obj->unlocked_get_login_name();
		info.phone_number       = obj->unlocked_get_phone_number();
		info.nick               = obj->unlocked_get_nick();
		info.password_hash      = obj->unlocked_get_password_hash();
		info.deal_password_hash = obj->unlocked_get_deal_password_hash();
		info.referrer_id        = AccountId(obj->get_referrer_id());
		info.level              = obj->get_level();
		info.max_level          = obj->get_max_level();
		info.subordinate_count  = obj->get_subordinate_count();
		info.performance        = obj->get_performance();
		info.flags              = obj->get_flags();
		info.banned_until       = obj->get_banned_until();
		info.created_time       = obj->get_created_time();
		info.created_ip         = obj->get_created_ip();
	}
}

bool AccountMap::has(AccountId account_id){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		return false;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		return false;
	}
	return true;
}
AccountMap::AccountInfo AccountMap::get(AccountId account_id){
	PROFILE_ME;

	AccountInfo info = { };
	info.account_id = account_id;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		return info;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		return info;
	}
	fill_account_info(info, it->obj);
	return info;
}
AccountMap::AccountInfo AccountMap::require(AccountId account_id){
	PROFILE_ME;

	auto info = get(account_id);
	if(Poseidon::has_none_flags_of(info.flags, FL_VALID)){
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return info;
}

std::uint64_t AccountMap::get_count(){
	return g_account_map->size();
}
void AccountMap::get_all(std::vector<AccountMap::AccountInfo> &ret, std::uint64_t begin, std::uint64_t max){
	PROFILE_ME;

	const auto size = g_account_map->size();
	if(begin >= size){
		return;
	}
	auto count = size - begin;
	if(count > max){
		count = max;
	}
	ret.reserve(ret.size() + count);
	auto it = g_account_map->begin();
	std::advance(it, static_cast<std::ptrdiff_t>(begin));
	for(std::uint64_t i = 0; i < count; ++it, ++i){
		AccountInfo info;
		fill_account_info(info, it->obj);
		ret.push_back(std::move(info));
	}
}

AccountMap::AccountInfo AccountMap::get_by_login_name(const std::string &login_name){
	PROFILE_ME;

	AccountInfo info = { };
	info.login_name = login_name;

	const auto it = g_account_map->find<1>(login_name);
	if(it == g_account_map->end<1>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: login_name = ", login_name);
		return info;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: login_name = ", login_name);
		return info;
	}
	fill_account_info(info, it->obj);
	return info;
}

void AccountMap::get_by_phone_number(std::vector<AccountMap::AccountInfo> &ret, const std::string &phone_number){
	PROFILE_ME;

	const auto range = g_account_map->equal_range<2>(phone_number);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
			LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", it->obj->get_account_id());
			continue;
		}
		AccountInfo info;
		fill_account_info(info, it->obj);
		ret.push_back(std::move(info));
	}
}
void AccountMap::get_by_referrer_id(std::vector<AccountMap::AccountInfo> &ret, AccountId referrer_id){
	PROFILE_ME;

	const auto range = g_account_map->equal_range<4>(referrer_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
			LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", it->obj->get_account_id());
			continue;
		}
		AccountInfo info;
		fill_account_info(info, it->obj);
		ret.push_back(std::move(info));
	}
}
void AccountMap::get_by_level(std::vector<AccountMap::AccountInfo> &ret, std::uint64_t level){
	PROFILE_ME;

	const auto range = g_account_map->equal_range<5>(level);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
			LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", it->obj->get_account_id());
			continue;
		}
		AccountInfo info;
		fill_account_info(info, it->obj);
		ret.push_back(std::move(info));
	}
}

std::string AccountMap::get_password_hash(const std::string &password){
	PROFILE_ME;

	auto salt = get_config<std::string>("password_salt");
	const auto sha256 = Poseidon::sha256_hash(password + std::move(salt));
	return Poseidon::Http::base64_encode(sha256.data(), sha256.size());
}

void AccountMap::set_login_name(AccountId account_id, std::string login_name){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_account_map->set_key<0, 1>(it, login_name);
	it->obj->set_login_name(std::move(login_name));
}
void AccountMap::set_phone_number(AccountId account_id, std::string phone_number){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_account_map->set_key<0, 2>(it, phone_number);
	it->obj->set_phone_number(std::move(phone_number));
}
void AccountMap::set_nick(AccountId account_id, std::string nick){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_account_map->set_key<0, 3>(it, nick);
	it->obj->set_nick(std::move(nick));
}
void AccountMap::set_password(AccountId account_id, const std::string &password){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_password_hash(get_password_hash(password));
}
void AccountMap::set_deal_password(AccountId account_id, const std::string &deal_password){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_deal_password_hash(get_password_hash(deal_password));
}
void AccountMap::set_level(AccountId account_id, std::uint64_t level){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_level(level);

	for(auto current_it = it; current_it != g_account_map->end<0>(); current_it = g_account_map->find<0>(current_it->referrer_id)){
		std::uint64_t new_max_level = 0;
		const auto range = g_account_map->equal_range<4>(current_it->account_id);
		for(auto subord_it = range.first; subord_it != range.second; ++subord_it){
			const auto max_level = subord_it->obj->get_max_level();
			if(new_max_level < max_level){
				new_max_level = max_level;
			}
		}
		const auto self_level = current_it->obj->get_level();
		if(new_max_level < self_level){
			new_max_level = self_level; // 现在是算自己的。
		}
		const auto old_max_level = current_it->obj->get_max_level();
		if(new_max_level == old_max_level){
			break;
		}
		LOG_EMPERY_PROMOTION_DEBUG("Updating max subordinate level: account_id = ", current_it->account_id,
			", old_max_level = ", old_max_level, ", new_max_level = ", new_max_level);
		current_it->obj->set_max_level(new_max_level);
	}
}
void AccountMap::set_flags(AccountId account_id, std::uint64_t flags){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
//	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
//		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
//		DEBUG_THROW(Exception, sslit("Account deleted"));
//	}

	it->obj->set_flags(flags);
}
void AccountMap::set_banned_until(AccountId account_id, std::uint64_t banned_until){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_banned_until(banned_until);
}
void AccountMap::accumulate_performance(AccountId account_id, std::uint64_t amount){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_id);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	const auto old_value = it->obj->get_performance();
	auto new_value = old_value + amount * 0.6;
	if(new_value < old_value){
		new_value = UINT64_MAX;
	}
	it->obj->set_performance(new_value);
}

AccountId AccountMap::create(std::string login_name, std::string phone_number, std::string nick,
	const std::string &password, const std::string &deal_password, AccountId referrer_id, std::uint64_t flags, std::string created_ip)
{
	PROFILE_ME;

	if(referrer_id){
		auto referrer_it = g_account_map->find<0>(referrer_id);
		for(;;){
			if(referrer_it == g_account_map->end<0>()){
				LOG_EMPERY_PROMOTION_DEBUG("No such referrer: referrer_id = ", referrer_id);
				DEBUG_THROW(Exception, sslit("No such referrer"));
			}
			const auto next_referrer_id = referrer_it->referrer_id;
			if(!next_referrer_id){
				break;
			}
			if(next_referrer_id == referrer_id){
				LOG_EMPERY_PROMOTION_ERROR("Circular referrer loop detected! referrer_id = ", referrer_id);
				DEBUG_THROW(Exception, sslit("Circular referrer loop detected"));
			}
			referrer_it = g_account_map->find<0>(next_referrer_id);
		}
	}

	auto it = g_account_map->find<1>(login_name);
	if(it != g_account_map->end<1>()){
		LOG_EMPERY_PROMOTION_DEBUG("Duplicate login_name: login_name = ", login_name);
		DEBUG_THROW(Exception, sslit("Duplicate login_name"));
	}

	auto account_id = g_account_map->empty() ? AccountId() : g_account_map->rbegin<0>()->account_id;
	do {
		++account_id;
	} while(g_account_map->find<0>(account_id) != g_account_map->end<0>());

	Poseidon::add_flags(flags, AccountMap::FL_VALID);
	const auto utc_now = Poseidon::get_utc_time();
	auto obj = boost::make_shared<MySql::Promotion_Account>(account_id.get(), std::move(login_name),
		std::move(phone_number), std::move(nick), get_password_hash(password), get_password_hash(deal_password),
		referrer_id.get(), 0, 0, 0, 0, flags, 0, utc_now, std::move(created_ip));
	obj->async_save(true);
	it = g_account_map->insert<1>(it, AccountElement(std::move(obj)));

	auto referrer_it = g_account_map->find<0>(referrer_id);
	while(referrer_it != g_account_map->end<0>()){
		const auto old_count = referrer_it->obj->get_subordinate_count();
		const auto new_count = old_count + 1;
		LOG_EMPERY_PROMOTION_DEBUG("Updating subordinate count: referrer_id = ", referrer_it->account_id,
			", old_count = ", old_count, ", new_count = ", new_count);
		referrer_it->obj->set_subordinate_count(new_count);
		referrer_it = g_account_map->find<0>(referrer_it->referrer_id);
	}

	return account_id;
}

const std::string &AccountMap::get_attribute(AccountId account_id, unsigned slot){
	PROFILE_ME;

	const auto it = g_attribute_map->find<1>(std::make_pair(account_id, slot));
	if(it == g_attribute_map->end<1>()){
		LOG_EMPERY_PROMOTION_TRACE("Account attribute not found: account_id = ", account_id, ", slot = ", slot);
		return Poseidon::EMPTY_STRING;
	}
	return it->obj->unlocked_get_value();
}
void AccountMap::get_attributes(std::vector<std::pair<unsigned, std::string>> &ret, AccountId account_id){
	PROFILE_ME;

	const auto range = g_attribute_map->equal_range<0>(account_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.push_back(std::make_pair(it->obj->get_slot(), it->obj->unlocked_get_value()));
	}
}
void AccountMap::touch_attribute(AccountId account_id, unsigned slot){
	PROFILE_ME;

	auto it = g_attribute_map->find<1>(std::make_pair(account_id, slot));
	if(it == g_attribute_map->end<1>()){
		it = g_attribute_map->insert<1>(it, AccountAttributeElement(
			boost::make_shared<MySql::Promotion_AccountAttribute>(account_id.get(), slot, std::string())));
		it->obj->async_save(true);
	}
}
void AccountMap::set_attribute(AccountId account_id, unsigned slot, std::string value){
	PROFILE_ME;

	const auto account_it = g_account_map->find<0>(account_id);
	if(account_it == g_account_map->end<0>()){
		LOG_EMPERY_PROMOTION_WARNING("No such account: account_id = ", account_id);
		DEBUG_THROW(Exception, sslit("No such account"));
	}

	auto it = g_attribute_map->find<1>(std::make_pair(account_id, slot));
	if(it == g_attribute_map->end<1>()){
		it = g_attribute_map->insert<1>(it, AccountAttributeElement(
			boost::make_shared<MySql::Promotion_AccountAttribute>(account_id.get(), slot, std::move(value))));
		it->obj->async_save(true);
	} else {
		it->obj->set_value(std::move(value));
	}
}

}
