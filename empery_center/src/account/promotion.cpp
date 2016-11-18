#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include <poseidon/json.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <iconv.h>
#include "../msg/err_account.hpp"
#include "../../../empery_promotion/src/msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/simple_http_client_daemon.hpp"
#include "../account.hpp"
#include "../checked_arithmetic.hpp"
#include "../account_attribute_ids.hpp"
#include "../events/account.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../msg/err_castle.hpp"
#include "../data/global.hpp"
#include "../buff_ids.hpp"

namespace EmperyCenter {

namespace {
	PlatformId  g_platform_id    = PlatformId(8500);

	std::string g_server_host    = "localhost";
	unsigned    g_server_port    = 6121;
	bool        g_server_use_ssl = false;
	std::string g_server_auth    = { };
	std::string g_server_path    = { };

	boost::container::flat_map<std::uint64_t, unsigned> g_level_config;

	std::string g_sms_host       = "localhost";
	unsigned    g_sms_port       = 80;
	bool        g_sms_use_ssl    = false;
	std::string g_sms_auth       = { };
	std::string g_sms_uri        = "/";
	std::string g_sms_message    = "code = $(code), minutes = $(minutes)";
	std::string g_sms_charset    = "UTF-8";

	std::pair<int, boost::shared_ptr<Account>> check_login_backtrace(
		const std::string &login_name, const std::string &password, const std::string &ip)
	{
		PROFILE_ME;

		Poseidon::OptionalMap get_params;
		get_params.set(sslit("loginName"), login_name);
		get_params.set(sslit("password"),  password);
		get_params.set(sslit("ip"),        ip);
		auto entity = SimpleHttpClientDaemon::sync_request(g_server_host, g_server_port, g_server_use_ssl,
			Poseidon::Http::V_GET, g_server_path + "/checkLoginBacktrace", std::move(get_params), g_server_auth);
		std::istringstream iss(entity.dump());
		auto response_object = Poseidon::JsonParser::parse_object(iss);
		LOG_EMPERY_CENTER_DEBUG("Promotion server response: ", response_object.dump());
		auto error_code = static_cast<int>(response_object.at(sslit("errorCode")).get<double>());
		boost::shared_ptr<Account> new_account;
		if(error_code != Msg::ST_OK){
			switch(error_code){
			case EmperyPromotion::Msg::ERR_NO_SUCH_ACCOUNT:
				error_code = Msg::ERR_NO_SUCH_ACCOUNT;
				break;
			case EmperyPromotion::Msg::ERR_INVALID_PASSWORD:
				error_code = Msg::ERR_INVALID_PASSWORD;
				break;
			case EmperyPromotion::Msg::ERR_ACCOUNT_BANNED:
				error_code = Msg::ERR_ACCOUNT_BANNED;
				break;
			default:
				LOG_EMPERY_CENTER_WARNING("Unexpected error code from promotion server: error_code = ", error_code);
				error_code = Msg::ERR_NO_SUCH_ACCOUNT;
				break;
			}
		}
		const auto referrers_it = response_object.find(sslit("referrers"));
		if(referrers_it != response_object.end()){
			const auto &referrers_array = referrers_it->second.get<Poseidon::JsonArray>();

			const auto check_account = [](AccountUuid referrer_uuid, const std::string &cur_login_name, const Poseidon::JsonObject &elem){
				const auto level = boost::lexical_cast<std::uint64_t>(elem.at(sslit("level")).get<std::string>());
				unsigned cur_level = 1;
				{
					auto level_it = g_level_config.upper_bound(level);
					if(level_it != g_level_config.begin()){
						--level_it;
						cur_level = level_it->second;
					}
				}
				const auto &cur_nick = elem.at(sslit("nick")).get<std::string>();
				const auto is_auction_center_enabled = elem.at(sslit("isAuctionCenterEnabled")).get<bool>();
		//		LOG_EMPERY_CENTER_DEBUG("Create or update account: cur_login_name = ", cur_login_name,
		//			", cur_level = ", cur_level, ", cur_nick = ", cur_nick, ", is_auction_center_enabled = ", is_auction_center_enabled);

				auto account = AccountMap::forced_reload_by_login_name(g_platform_id, cur_login_name);
				if(!account){
					const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
					const auto utc_now = Poseidon::get_utc_time();
					LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid,
						", cur_login_name = ", cur_login_name, ", is_auction_center_enabled = ", is_auction_center_enabled);
					auto pair = Account::async_create(account_uuid, g_platform_id, cur_login_name, referrer_uuid, cur_level, utc_now, cur_nick);
					Poseidon::JobDispatcher::yield(pair.first, true);

					account = std::move(pair.second);
					AccountMap::insert(account, std::string());
				} else {
					account->set_promotion_level(cur_level);
				}
				if(is_auction_center_enabled){
					boost::container::flat_map<AccountAttributeId, std::string> modifiers;
					modifiers.emplace(AccountAttributeIds::ID_AUCTION_CENTER_ENABLED, "1");
					account->set_attributes(std::move(modifiers));
				}
				return account;
			};

			AccountUuid referrer_uuid;
			for(auto it = referrers_array.rbegin(); it != referrers_array.rend(); ++it){
				const auto &elem = it->get<Poseidon::JsonObject>();
				const auto &referrer_login_name = elem.at(sslit("loginName")).get<std::string>();
				const auto referrer = check_account(referrer_uuid, referrer_login_name, elem);
				referrer_uuid = referrer->get_account_uuid();
			}
			new_account = check_account(referrer_uuid, login_name, response_object);
		}
		return std::make_pair(error_code, new_account);
	}

	struct IconvCloer {
		::iconv_t operator()() const noexcept {
			return (::iconv_t)-1;
		}
		void operator()(::iconv_t cd) const noexcept {
			const auto ret = ::iconv_close(cd);
			if(ret != 0){
				const int err_code = errno;
				LOG_EMPERY_CENTER_FATAL("::iconv_close() failed: err_code = ", err_code);
			}
		}
	};

	void send_verification_code(const std::string &login_name, const std::string &verification_code, std::uint64_t expiry_duration){
		PROFILE_ME;

		Poseidon::OptionalMap get_params;
		get_params.set(sslit("loginName"), login_name);
		auto entity = SimpleHttpClientDaemon::sync_request(g_server_host, g_server_port, g_server_use_ssl,
			Poseidon::Http::V_GET, g_server_path + "/queryAccountAttributes", std::move(get_params), g_server_auth);
		std::istringstream iss(entity.dump());
		auto response_object = Poseidon::JsonParser::parse_object(iss);
		LOG_EMPERY_CENTER_DEBUG("Promotion server response: ", response_object.dump());
		auto error_code = static_cast<int>(response_object.at(sslit("errorCode")).get<double>());
		if(error_code != Msg::ST_OK){
			LOG_EMPERY_CENTER_WARNING("Error resposne from promotion server: ", response_object.dump());
			return;
		}
		auto phone_number = std::move(response_object.at(sslit("phoneNumber")).get<std::string>());
		{
			auto wit = phone_number.begin();
			for(auto rit = wit; rit != phone_number.end(); ++rit){
				if((*rit < '0') || ('9' < *rit)){
					continue;
				}
				*wit = *rit;
				++wit;
			}
			phone_number.erase(wit, phone_number.end());
		}
		LOG_EMPERY_CENTER_DEBUG("Got phone number: login_name = ", login_name, ", phone_number = ", phone_number);

		auto utf8_message = g_sms_message;
		auto pos = utf8_message.find("$(code)");
		if(pos != std::string::npos){
			utf8_message.replace(pos, sizeof("$(code)") - 1, verification_code);
		}
		pos = utf8_message.find("$(minutes)");
		if(pos != std::string::npos){
			char str[256];
			unsigned len = (unsigned)std::sprintf(str, "%.f", std::ceil(expiry_duration / 60000.0 - 0.001));
			utf8_message.replace(pos, sizeof("$(minutes)") - 1, str, len);
		}
		LOG_EMPERY_CENTER_DEBUG("Generated message: utf8_message = ", utf8_message);

		const Poseidon::UniqueHandle<IconvCloer> cd(::iconv_open(g_sms_charset.c_str(), "UTF-8"));
		if(!cd){
			const int err_code = errno;
			LOG_EMPERY_CENTER_ERROR("::iconv_open() failed: err_code = ", err_code);
			return;
		}
		std::string message(4096, '*');
		char *inbuf = &utf8_message[0];
		std::size_t inbytes = utf8_message.size();
		char *outbuf = &message[0];
		std::size_t outbyes = message.size();
		const auto characters = ::iconv(cd.get(), &inbuf, &inbytes, &outbuf, &outbyes);
		if(characters == (std::size_t)-1){
			const int err_code = errno;
			LOG_EMPERY_CENTER_ERROR("::iconv() failed: err_code = ", err_code);
			return;
		}
		message.erase(message.begin() + (outbuf - &message[0]), message.end());
		LOG_EMPERY_CENTER_DEBUG("Converted message: message = ", message);

		auto uri = g_sms_uri;
		pos = uri.find("$(phone)");
		if(pos != std::string::npos){
			uri.replace(pos, sizeof("$(phone)") - 1, Poseidon::Http::url_encode(phone_number));
		}
		pos = uri.find("$(message)");
		if(pos != std::string::npos){
			uri.replace(pos, sizeof("$(message)") - 1, Poseidon::Http::url_encode(message));
		}

		SimpleHttpClientDaemon::sync_request(g_sms_host, g_sms_port, g_sms_use_ssl,
			Poseidon::Http::V_GET, std::move(uri), { }, g_sms_auth);
	}

	int set_password(const std::string &login_name, const std::string &password){
		PROFILE_ME;

		Poseidon::OptionalMap get_params;
		get_params.set(sslit("loginName"), login_name);
		get_params.set(sslit("password"),  password);
		auto entity = SimpleHttpClientDaemon::sync_request(g_server_host, g_server_port, g_server_use_ssl,
			Poseidon::Http::V_GET, g_server_path + "/queryAccountAttributes", std::move(get_params), g_server_auth);
		std::istringstream iss(entity.dump());
		auto response_object = Poseidon::JsonParser::parse_object(iss);
		LOG_EMPERY_CENTER_DEBUG("Promotion server response: ", response_object.dump());
		auto error_code = static_cast<int>(response_object.at(sslit("errorCode")).get<double>());
		if(error_code != Msg::ST_OK){
			switch(error_code){
			case EmperyPromotion::Msg::ERR_NO_SUCH_ACCOUNT:
				error_code = Msg::ERR_NO_SUCH_ACCOUNT;
				break;
			default:
				LOG_EMPERY_CENTER_WARNING("Unexpected error code from promotion server: error_code = ", error_code);
				error_code = Msg::ERR_NO_SUCH_ACCOUNT;
				break;
			}
		}
		return error_code;
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		get_config(g_platform_id,    "promotion_platform_id");

		get_config(g_server_host,    "promotion_server_host");
		get_config(g_server_port,    "promotion_server_port");
		get_config(g_server_use_ssl, "promotion_server_use_ssl");
		get_config(g_server_auth,    "promotion_server_auth_user_pass");
		get_config(g_server_path,    "promotion_server_path");

		Poseidon::CsvParser csv;

		boost::container::flat_map<std::uint64_t, unsigned> level_config;
		level_config.reserve(20);
		constexpr char path[] = "empery_promotion_levels.csv";
		LOG_EMPERY_CENTER_INFO("Loading csv file: path = ", path);
		csv.load(path);
		while(csv.fetch_row()){
			std::uint64_t key;
			unsigned level;

			csv.get(key,   "level");
			csv.get(level, "displayLevel");

			if(!level_config.emplace(key, level).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate promotion level: key = ", key);
				DEBUG_THROW(Exception, sslit("Duplicate promotion level"));
			}
		}
		g_level_config = std::move(level_config);

		get_config(g_sms_host,    "promotion_sms_host");
		get_config(g_sms_port,    "promotion_sms_port");
		get_config(g_sms_use_ssl, "promotion_sms_use_ssl");
		get_config(g_sms_auth,    "promotion_sms_auth_user_pass");
		get_config(g_sms_uri,     "promotion_sms_uri");
		get_config(g_sms_message, "promotion_sms_message");
		get_config(g_sms_charset, "promotion_sms_charset");

		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountSynchronizeWithThirdServer>(
			[](const boost::shared_ptr<Events::AccountSynchronizeWithThirdServer> &event){
				PROFILE_ME;
				if(event->platform_id != g_platform_id){
					return;
				}
				LOG_EMPERY_CENTER_DEBUG("Updating account level from promotion server: login_name = ", event->login_name);
				auto result = check_login_backtrace(event->login_name, event->third_token, std::string());
				if(event->error_code){
					*(event->error_code) = result.first;
				}
			});
		LOG_EMPERY_CENTER_DEBUG("Created AccountSynchronizeWithThirdServer listener.");
		handles.push(std::move(listener));
	}
}

ACCOUNT_SERVLET("promotion/check_login", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &password   = params.at("password");
	const auto &token      = params.at("token");

	const auto result = check_login_backtrace(login_name, password, session->get_remote_info().ip.get());
	if(result.first != Msg::ST_OK){
		return Response(result.first) <<login_name;
	}
	const auto &account = result.second;
	if(!account){
		LOG_EMPERY_CENTER_ERROR("Null account pointer! login_name = ", login_name);
		DEBUG_THROW(Exception, sslit("Null account pointer"));
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<std::uint64_t>("account_token_expiry_duration", 0);

	const auto token_expiry_time = saturated_add(utc_now, token_expiry_duration);

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(4);
	modifiers[AccountAttributeIds::ID_LOGIN_TOKEN]             = token;
	modifiers[AccountAttributeIds::ID_LOGIN_TOKEN_EXPIRY_TIME] = boost::lexical_cast<std::string>(token_expiry_time);
	modifiers[AccountAttributeIds::ID_SAVED_THIRD_TOKEN]       = password;
	account->set_attributes(std::move(modifiers));

	root[sslit("hasBeenActivated")]    = account->has_been_activated();
	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("promotion/renewal_token", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &old_token  = params.at("oldToken");
	const auto &token      = params.at("token");

	const auto result = check_login_backtrace(login_name, std::string(), session->get_remote_info().ip.get());
	if((result.first != Msg::ST_OK) && (result.first != Msg::ERR_INVALID_PASSWORD)){
		return Response(result.first) <<login_name;
	}
	const auto &account = result.second;
	if(!account){
		LOG_EMPERY_CENTER_ERROR("Null account pointer! login_name = ", login_name);
		DEBUG_THROW(Exception, sslit("Null account pointer"));
	}
	if(old_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	const auto expected_token = account->get_attribute(AccountAttributeIds::ID_LOGIN_TOKEN);
	if(old_token != expected_token){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", expected_token, ", got ", old_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<std::uint64_t>("account_token_expiry_duration", 0);

	const auto token_expiry_time = saturated_add(utc_now, token_expiry_duration);

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(4);
	modifiers[AccountAttributeIds::ID_LOGIN_TOKEN]             = token;
	modifiers[AccountAttributeIds::ID_LOGIN_TOKEN_EXPIRY_TIME] = boost::lexical_cast<std::string>(token_expiry_time);
	account->set_attributes(std::move(modifiers));

	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("promotion/regain", root, session, params){
	const auto &login_name = params.at("loginName");

	const auto result = check_login_backtrace(login_name, std::string(), session->get_remote_info().ip.get());
	if((result.first != Msg::ST_OK) && (result.first != Msg::ERR_INVALID_PASSWORD)){
		return Response(result.first) <<login_name;
	}
	const auto &account = result.second;
	if(!account){
		LOG_EMPERY_CENTER_ERROR("Null account pointer! login_name = ", login_name);
		DEBUG_THROW(Exception, sslit("Null account pointer"));
	}
	const auto utc_now = Poseidon::get_utc_time();
	const auto old_cooldown = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIF_CODE_COOLDOWN);
	if(old_cooldown > utc_now){
		return Response(Msg::ERR_VERIFICATION_CODE_FLOOD) <<(old_cooldown - utc_now);
	}

	char str[64];
	unsigned len = (unsigned)std::sprintf(str, "%06u", (unsigned)(Poseidon::random_uint64() % 1000000));
	auto verification_code = std::string(str + len - 6, str + len);
	LOG_EMPERY_CENTER_DEBUG("Generated verification code: login_name = ", login_name, ", verification_code = ", verification_code);

	const auto expiry_duration = get_config<std::uint64_t>("promotion_verification_code_expiry_duration", 900000);
	auto expiry_time = boost::lexical_cast<std::string>(saturated_add(utc_now, expiry_duration));

	const auto cooldown_duration = get_config<std::uint64_t>("promotion_verification_code_cooldown_duration", 120000);
	auto cooldown = boost::lexical_cast<std::string>(saturated_add(utc_now, cooldown_duration));

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(3);
	modifiers.emplace(AccountAttributeIds::ID_VERIF_CODE,             verification_code);
	modifiers.emplace(AccountAttributeIds::ID_VERIF_CODE_EXPIRY_TIME, expiry_time);
	modifiers.emplace(AccountAttributeIds::ID_VERIF_CODE_COOLDOWN,    cooldown);
	account->set_attributes(std::move(modifiers));

	send_verification_code(login_name, verification_code, expiry_duration);

	root[sslit("verificationCodeExpiryDuration")] = expiry_duration;
	root[sslit("verificationCodeCoolDown")]       = cooldown_duration;

	return Response();
}

ACCOUNT_SERVLET("promotion/reset_password", root, session, params){
	const auto &login_name        = params.at("loginName");
	const auto &verification_code = params.at("verificationCode");
	const auto &new_password      = params.at("newPassword");

	const auto account = AccountMap::get_or_reload_by_login_name(g_platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	const auto expiry_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIF_CODE_EXPIRY_TIME);
	if(expiry_time <= utc_now){
		return Response(Msg::ERR_VERIFICATION_CODE_EXPIRED);
	}
	const auto &code_expected = account->get_attribute(AccountAttributeIds::ID_VERIF_CODE);
	if(verification_code != code_expected){
		LOG_EMPERY_CENTER_DEBUG("Unexpected verification code: expecting ", code_expected, ", got ", verification_code);
		return Response(Msg::ERR_VERIFICATION_CODE_INCORRECT);
	}

	const int error_code = set_password(login_name, new_password);
	if(error_code != Msg::ST_OK){
		return Response(error_code) <<login_name;
	}

	return Response();
}

ACCOUNT_SERVLET("promotion/activate", root, session, params){
	const auto &login_name   = params.at("loginName");
	const auto &initial_nick = params.get("initialNick");

	const auto account = AccountMap::get_or_reload_by_login_name(g_platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	if(account->has_been_activated()){
		return Response(Msg::ERR_ACCOUNT_ALREADY_ACTIVATED);
	}
	const auto account_uuid = account->get_account_uuid();

	if(!initial_nick.empty()){
		std::vector<boost::shared_ptr<Account>> other_accounts;
		AccountMap::get_by_nick(other_accounts, initial_nick);
		for(auto it = other_accounts.begin(); it != other_accounts.end(); ++it){
			const auto &other_account = *it;
			if(other_account != account){
				LOG_EMPERY_CENTER_DEBUG("Nick conflict: initial_nick = ", initial_nick, ", account_uuid = ", account_uuid,
					", other_nick = ", other_account->get_nick(), ", other_account_uuid = ", other_account->get_account_uuid());
				return Response(Msg::ERR_NICK_CONFLICT) <<other_account->get_nick();
			}
		}
		account->set_nick(initial_nick);
	}

	auto primary_castle = WorldMap::get_primary_castle(account_uuid);
	if(!primary_castle){
		LOG_EMPERY_CENTER_INFO("Creating initial castle: account_uuid = ", account_uuid);
		primary_castle = WorldMap::place_castle_random(
			[&](Coord coord){
				const auto castle_uuid = MapObjectUuid(Poseidon::Uuid::random());
				const auto utc_now = Poseidon::get_utc_time();

				const auto protection_minutes = Data::Global::as_unsigned(Data::Global::SLOT_NOVICIATE_PROTECTION_DURATION);
				const auto protection_duration = saturated_mul<std::uint64_t>(protection_minutes, 60000);

				auto castle = boost::make_shared<Castle>(castle_uuid, account_uuid, MapObjectUuid(), account->get_nick(), coord, utc_now);
				castle->set_buff(BuffIds::ID_NOVICIATE_PROTECTION, utc_now, protection_duration);
				return castle;
			});
		if(!primary_castle){
			return Response(Msg::ERR_NO_START_POINTS_AVAILABLE);
		}
	}

	account->set_activated(true);

	return Response();
}

}
