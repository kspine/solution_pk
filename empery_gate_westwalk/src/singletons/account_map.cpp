#include "../precompiled.hpp"
#include "account_map.hpp"
#include <string.h>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../mysql/account.hpp"

namespace EmperyGateWestwalk {

namespace {
	struct StrCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

	struct AccountElement {
		boost::shared_ptr<MySql::Westwalk_Account> obj;

		std::string account_name;

		explicit AccountElement(boost::shared_ptr<MySql::Westwalk_Account> obj_)
			: obj(std::move(obj_))
			, account_name(obj->unlocked_get_account_name())
		{
		}
	};

	MULTI_INDEX_MAP(AccountMapContainer, AccountElement,
		UNIQUE_MEMBER_INDEX(account_name, StrCaseComparator)
	)

	boost::shared_ptr<AccountMapContainer> g_account_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto account_map = boost::make_shared<AccountMapContainer>();
		LOG_EMPERY_GATE_WESTWALK_INFO("Loading accounts...");
		conn->execute_sql("SELECT * FROM `Westwalk_Account`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Westwalk_Account>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			account_map->insert(AccountElement(std::move(obj)));
		}
		LOG_EMPERY_GATE_WESTWALK_INFO("Loaded ", account_map->size(), " account(s).");
		g_account_map = account_map;
		handles.push(account_map);
	}

	void fill_account_info(AccountMap::AccountInfo &info, const boost::shared_ptr<MySql::Westwalk_Account> &obj){
		PROFILE_ME;

		info.account_name                  = obj->unlocked_get_account_name();
		info.created_ip                    = obj->unlocked_get_created_ip();
		info.created_time                  = obj->get_created_time();
		info.remarks                      = obj->unlocked_get_remarks();
		info.password_hash                 = obj->unlocked_get_password_hash();
		info.disposable_password_hash       = obj->unlocked_get_disposable_password_hash();
		info.disposable_password_expiry_time = obj->get_disposable_password_expiry_time();
		info.password_regain_cooldown_time   = obj->get_password_regain_cooldown_time();
		info.token                        = obj->unlocked_get_token();
		info.token_expiry_time              = obj->get_token_expiry_time();
		info.last_login_ip                  = obj->unlocked_get_last_login_ip();
		info.last_login_time                = obj->get_last_login_time();
		info.banned_until                  = obj->get_banned_until();
		info.retry_count                   = obj->get_retry_count();
		info.flags                        = obj->get_flags();
	}
}

bool AccountMap::has(const std::string &account_name){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		return false;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		return false;
	}
	return true;
}
AccountMap::AccountInfo AccountMap::get(const std::string &account_name){
	PROFILE_ME;

	AccountInfo info = { };
	info.account_name = account_name;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		return info;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		return info;
	}
	fill_account_info(info, it->obj);
	return info;
}
AccountMap::AccountInfo AccountMap::require(const std::string &account_name){
	PROFILE_ME;

	auto info = get(account_name);
	if(Poseidon::has_none_flags_of(info.flags, FL_VALID)){
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return info;
}
void AccountMap::get_all(std::vector<AccountMap::AccountInfo> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(g_account_map->begin(), g_account_map->end())));
	for(auto it = g_account_map->begin(); it != g_account_map->end(); ++it){
		AccountInfo info;
		fill_account_info(info, it->obj);
		ret.emplace_back(std::move(info));
	}
}

std::string AccountMap::random_password(){
	PROFILE_ME;

	const auto chars = get_config<std::string>("random_password_chars", "0123456789");
	const auto length = get_config<std::size_t>("random_password_length", 6);

	std::string str;
	str.resize(length);
	for(auto it = str.begin(); it != str.end(); ++it){
		*it = chars.at(Poseidon::rand32() % chars.size());
	}
	return str;
}
std::string AccountMap::get_password_hash(const std::string &password){
	PROFILE_ME;

	const auto salt = get_config<std::string>("password_salt");
	const auto sha256 = Poseidon::sha256_hash(password + salt);
	return Poseidon::Http::base64_encode(sha256.data(), sha256.size());
}

void AccountMap::set_password(const std::string &account_name, const std::string &password){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_password_hash(get_password_hash(password));
}
void AccountMap::set_disposable_password(const std::string &account_name, const std::string &password, boost::uint64_t expiry_time){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_disposable_password_hash(get_password_hash(password));
	it->obj->set_disposable_password_expiry_time(expiry_time);
}
void AccountMap::commit_disposable_password(const std::string &account_name){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_password_hash(it->obj->unlocked_get_disposable_password_hash());
	it->obj->set_disposable_password_hash(VAL_INIT);
	it->obj->set_disposable_password_expiry_time(0);
}
void AccountMap::set_password_regain_cooldown_time(const std::string &account_name, boost::uint64_t expiry_time){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_password_regain_cooldown_time(expiry_time);
}
void AccountMap::set_token(const std::string &account_name, std::string token, boost::uint64_t expiry_time){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_token(std::move(token));
	it->obj->set_token_expiry_time(expiry_time);
}
void AccountMap::set_last_login(const std::string &account_name, std::string last_login_ip, boost::uint64_t last_login_time){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_last_login_ip(std::move(last_login_ip));
	it->obj->set_last_login_time(last_login_time);
}
void AccountMap::set_banned_until(const std::string &account_name, boost::uint64_t banned_until){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}
	it->obj->set_banned_until(banned_until);
}
boost::uint32_t AccountMap::decrement_password_retry_count(const std::string &account_name){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}
	auto count = it->obj->get_retry_count();
	if(count > 0){
		--count;
	}
	it->obj->set_retry_count(count);

	if(count == 0){
		auto flags = it->obj->get_flags();
		Poseidon::add_flags(flags, FL_FROZEN);
		it->obj->set_flags(flags);
	}

	return count;
}
boost::uint32_t AccountMap::reset_password_retry_count(const std::string &account_name){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_name);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account not found: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Account deleted: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}
	const auto password_retry_count = get_config<boost::uint32_t>("password_retry_count", 10);
	it->obj->set_retry_count(password_retry_count);
	return password_retry_count;
}

void AccountMap::create(std::string account_name, std::string token, std::string remote_ip, std::string remarks, boost::uint64_t flags){
	PROFILE_ME;

	auto it = g_account_map->find<0>(account_name);
	if(it != g_account_map->end<0>()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Duplicate account name: account_name = ", account_name);
		DEBUG_THROW(Exception, sslit("Duplicate account name"));
	}

	const auto token_expiry_duration = get_config<boost::uint64_t>("token_expiry_duration", 604800000);

	Poseidon::add_flags(flags, AccountMap::FL_VALID);
	const auto utc_now = Poseidon::get_utc_time();
	const auto password_retry_count = get_config<boost::uint32_t>("password_retry_count", 10);
	auto obj = boost::make_shared<MySql::Westwalk_Account>(std::move(account_name), std::move(remote_ip), utc_now,
		std::move(remarks), std::string() /* get_password_hash(random_password()) */, std::string(), 0, 0,
		std::move(token), utc_now + token_expiry_duration, std::string(), 0, 0, password_retry_count, flags);
	obj->async_save(true);
	it = g_account_map->insert<0>(it, AccountElement(std::move(obj)));
}

}
