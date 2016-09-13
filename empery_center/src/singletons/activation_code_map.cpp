#include "../precompiled.hpp"
#include "activation_code_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include "../activation_code.hpp"
#include "../mysql/activation_code.hpp"
#include "../string_utilities.hpp"

namespace EmperyCenter {

namespace {
	struct ActivationCodeElement {
		boost::shared_ptr<ActivationCode> activation_code;

		std::size_t code_hash;
		std::uint64_t expiry_time;

		explicit ActivationCodeElement(boost::shared_ptr<ActivationCode> activation_code_)
			: activation_code(std::move(activation_code_))
			, code_hash(hash_string_nocase(activation_code->get_code()))
			, expiry_time(activation_code->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(ActivationCodeContainer, ActivationCodeElement,
		MULTI_MEMBER_INDEX(code_hash)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<ActivationCodeContainer> g_activation_code_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Activation code gc timer: now = ", now);

		const auto activation_code_map = g_activation_code_map.lock();
		if(!activation_code_map){
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();

		for(;;){
			const auto it = activation_code_map->begin<1>();
			if(it == activation_code_map->end<1>()){
				break;
			}
			if(utc_now < it->expiry_time){
				break;
			}

			const auto activation_code = it->activation_code;
			LOG_EMPERY_CENTER_DEBUG("Reclaiming activation code: code = ", activation_code->get_code());
			activation_code_map->erase<1>(it);
		}
	}

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto activation_code_map = boost::make_shared<ActivationCodeContainer>();
		LOG_EMPERY_CENTER_INFO("Loading activation codes...");
		std::ostringstream oss;
		const auto utc_now = Poseidon::get_utc_time();
		oss <<"SELECT * FROM `Center_ActivationCode` WHERE `expiry_time` > " <<Poseidon::MySql::DateTimeFormatter(utc_now);
		conn->execute_sql(oss.str());
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_ActivationCode>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto activation_code = boost::make_shared<ActivationCode>(std::move(obj));
			activation_code_map->insert(ActivationCodeElement(std::move(activation_code)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", activation_code_map->size(), " activation code(s).");
		g_activation_code_map = activation_code_map;
		handles.push(activation_code_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<ActivationCode> ActivationCodeMap::get(const std::string &code){
	PROFILE_ME;

	const auto activation_code_map = g_activation_code_map.lock();
	if(!activation_code_map){
		LOG_EMPERY_CENTER_WARNING("Activation code map not loaded.");
		return { };
	}

	const auto range = activation_code_map->equal_range<0>(hash_string_nocase(code));
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->activation_code->get_code(), code)){
			return it->activation_code;
		}
	}
	LOG_EMPERY_CENTER_TRACE("Activation code not found: code = ", code);
	return { };
}
boost::shared_ptr<ActivationCode> ActivationCodeMap::require(const std::string &code){
	PROFILE_ME;

	auto ret = get(code);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Activation code not found: code = ", code);
		DEBUG_THROW(Exception, sslit("Activation code not found"));
	}
	return ret;
}

void ActivationCodeMap::get_all(std::vector<boost::shared_ptr<ActivationCode>> &ret){
	PROFILE_ME;

	const auto activation_code_map = g_activation_code_map.lock();
	if(!activation_code_map){
		LOG_EMPERY_CENTER_WARNING("Activation code map not loaded.");
		return;
	}

	ret.reserve(ret.size() + activation_code_map->size());
	for(auto it = activation_code_map->begin(); it != activation_code_map->end(); ++it){
		ret.emplace_back(it->activation_code);
	}
}

void ActivationCodeMap::insert(const boost::shared_ptr<ActivationCode> &activation_code){
	PROFILE_ME;

	const auto activation_code_map = g_activation_code_map.lock();
	if(!activation_code_map){
		LOG_EMPERY_CENTER_WARNING("Activation code map not loaded.");
		DEBUG_THROW(Exception, sslit("Activation code map not loaded"));
	}

	const auto &code = activation_code->get_code();

	LOG_EMPERY_CENTER_DEBUG("Inserting activation code: code = ", code);
	const auto result = activation_code_map->insert(ActivationCodeElement(activation_code));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Activation code already exists: code = ", code);
		DEBUG_THROW(Exception, sslit("Activation code already exists"));
	}
}
void ActivationCodeMap::update(const boost::shared_ptr<ActivationCode> &activation_code, bool throws_if_not_exists){
	PROFILE_ME;

	const auto activation_code_map = g_activation_code_map.lock();
	if(!activation_code_map){
		LOG_EMPERY_CENTER_WARNING("Activation code map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Activation code map not loaded"));
		}
		return;
	}

	const auto &code = activation_code->get_code();

	const auto range = activation_code_map->equal_range<0>(hash_string_nocase(code));
	auto it = range.second;
	for(auto tit = range.first; tit != range.second; ++tit){
		if(are_strings_equal_nocase(tit->activation_code->get_code(), code)){
			it = tit;
			break;
		}
	}
	if(it == range.second){
		LOG_EMPERY_CENTER_WARNING("Activation code not found: code = ", code);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Activation code not found"));
		}
		return;
	}
	if(it->activation_code != activation_code){
		LOG_EMPERY_CENTER_DEBUG("Activation code expired: code = ", code);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating activation code: code = ", code);
	const auto utc_now = Poseidon::get_utc_time();
	if(activation_code->get_expiry_time() < utc_now){
		activation_code_map->erase<0>(it);
	} else {
		activation_code_map->replace<0>(it, ActivationCodeElement(activation_code));
	}
}
void ActivationCodeMap::remove(const std::string &code) noexcept {
	PROFILE_ME;

	const auto activation_code_map = g_activation_code_map.lock();
	if(!activation_code_map){
		LOG_EMPERY_CENTER_WARNING("Activation code map not loaded.");
		return;
	}

	const auto range = activation_code_map->equal_range<0>(hash_string_nocase(code));
	auto it = range.second;
	for(auto tit = range.first; tit != range.second; ++tit){
		if(are_strings_equal_nocase(tit->activation_code->get_code(), code)){
			it = tit;
			break;
		}
	}
	if(it == range.second){
		LOG_EMPERY_CENTER_WARNING("Activation code not found: code = ", code);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Removing activation code: code = ", code);
	activation_code_map->erase<0>(it);
}

}
