#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/dns_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/http/client.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/json.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/async_job.hpp>
#include <boost/container/flat_map.hpp>
#include <iconv.h>
#include "../msg/err_account.hpp"
#include "../../../empery_promotion/src/msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../checked_arithmetic.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../activation_code.hpp"
#include "../account_attribute_ids.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

constexpr auto PLATFORM_ID = PlatformId(8500);

namespace {
	class SimpleHttpClient : public Poseidon::Http::Client {
	private:
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;

		Poseidon::Http::ResponseHeaders m_headers;
		Poseidon::StreamBuffer m_entity;

	public:
		SimpleHttpClient(const Poseidon::SockAddr &addr, bool use_ssl, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::Http::Client(addr, use_ssl)
			, m_promise(std::move(promise))
			, m_headers(), m_entity()
		{
		}
		~SimpleHttpClient(){
		}

	protected:
		void on_close(int err_code) noexcept override {
			PROFILE_ME;

			try {
				Poseidon::enqueue_async_job(
					boost::weak_ptr<void>(virtual_weak_from_this<SimpleHttpClient>()),
					std::bind(
						[](const boost::shared_ptr<Poseidon::JobPromise> &promise){
							if(!promise->is_satisfied()){
								promise->set_exception(boost::copy_exception(std::runtime_error("Lost connection to remote server")));
							}
						},
						m_promise
					)
				);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
			}

			Poseidon::Http::Client::on_close(err_code);
		}

		void on_sync_response_headers(Poseidon::Http::ResponseHeaders response_headers,
			std::string /* transfer_encoding */, std::uint64_t /* content_length */) override
		{
			PROFILE_ME;

			m_headers = std::move(response_headers);
		}
		void on_sync_response_entity(std::uint64_t /* entity_offset */, bool /* is_chunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;

			m_entity.splice(entity);
		}
		void on_sync_response_end(std::uint64_t /* content_length */, bool /* is_chunked */, Poseidon::OptionalMap /* headers */) override {
			PROFILE_ME;

			LOG_EMPERY_CENTER_DEBUG("Received response from remote server: status_code = ", m_headers.status_code, ", entity = ", m_entity.dump());
			if(m_headers.status_code / 100 == 2){
				m_promise->set_success();
			} else {
				LOG_EMPERY_CENTER_ERROR("Received error response from remote server: status_code = ", m_headers.status_code);
				m_promise->set_exception(boost::copy_exception(std::runtime_error("Error response from remote server")));
			}
		}

	public:
		void send(std::string uri, Poseidon::OptionalMap get_params, Poseidon::OptionalMap headers){
			PROFILE_ME;

			Poseidon::Http::RequestHeaders req_headers;
			req_headers.verb       = Poseidon::Http::V_GET;
			req_headers.uri        = std::move(uri);
			req_headers.version    = 10001;
			req_headers.get_params = std::move(get_params);
			req_headers.headers    = std::move(headers);
			if(!Poseidon::Http::Client::send(std::move(req_headers))){
				LOG_EMPERY_CENTER_WARNING("Failed to send data to remote server!");
				DEBUG_THROW(Exception, sslit("Failed to send data to remote server"));
			}
		}

		const Poseidon::StreamBuffer &get_entity() const {
			return m_entity;
		}
		Poseidon::StreamBuffer &get_entity(){
			return m_entity;
		}
	};

	std::string g_server_host    = "localhost";
	unsigned    g_server_port    = 6121;
	bool        g_server_use_ssl = false;
	std::string g_server_auth    = { };
	std::string g_server_path    = { };

	boost::container::flat_map<std::string, unsigned> g_level_config;

	std::string g_sms_host       = "localhost";
	unsigned    g_sms_port       = 80;
	bool        g_sms_use_ssl    = false;
	std::string g_sms_auth       = { };
	std::string g_sms_uri        = "/";
	std::string g_sms_message    = "code = $(code), minutes = $(minutes)";
	std::string g_sms_charset    = "UTF-8";

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		get_config(g_server_host,    "promotion_server_host");
		get_config(g_server_port,    "promotion_server_port");
		get_config(g_server_use_ssl, "promotion_server_use_ssl");
		get_config(g_server_auth,    "promotion_server_auth_user_pass");
		get_config(g_server_path,    "promotion_server_path");

		Poseidon::CsvParser csv;

		boost::container::flat_map<std::string, unsigned> level_config;
		level_config.reserve(20);
		csv.load("empery_promotion_levels.csv");
		while(csv.fetch_row()){
			std::string key;
			unsigned level;

			csv.get(key,   "level");
			csv.get(level, "displayLevel");

			if(!level_config.emplace(std::move(key), level).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate promotion level: key = ", key);
				DEBUG_THROW(Exception, sslit("Duplicate promotion level"));
			}
		}
		g_level_config = std::move(level_config);

		get_config(g_sms_host,       "promotion_sms_host");
		get_config(g_sms_port,       "promotion_sms_port");
		get_config(g_sms_use_ssl,    "promotion_sms_use_ssl");
		get_config(g_sms_auth,       "promotion_sms_auth_user_pass");
		get_config(g_sms_uri,        "promotion_sms_uri");
		get_config(g_sms_message,    "promotion_sms_message");
		get_config(g_sms_charset,    "promotion_sms_charset");
	}

	int check_login_backtrace(boost::shared_ptr<Account> &new_account,
		const std::string &login_name, const std::string &password, const std::string &ip)
	{
		PROFILE_ME;

		const auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
		{
			const auto promise = Poseidon::DnsDaemon::async_lookup(sock_addr, g_server_host, g_server_port);
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}

		boost::shared_ptr<SimpleHttpClient> client;
		{
			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			client = boost::make_shared<SimpleHttpClient>(*sock_addr, g_server_use_ssl, promise);
			client->go_resident();

			Poseidon::OptionalMap get_params;
			get_params.set(sslit("loginName"), login_name);
			get_params.set(sslit("password"),  password);
			get_params.set(sslit("ip"),        ip);

			Poseidon::OptionalMap headers;
			headers.set(sslit("Host"), g_server_host);
			headers.set(sslit("Connection"), "Close");
			if(!g_server_auth.empty()){
				headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(g_server_auth));
			}
			client->send(g_server_path + "/checkLoginBacktrace", std::move(get_params), std::move(headers));
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}
		std::istringstream iss(client->get_entity().dump());
		auto response_object = Poseidon::JsonParser::parse_object(iss);
		LOG_EMPERY_CENTER_DEBUG("Promotion server response: ", response_object.dump());
		auto error_code = static_cast<int>(response_object.at(sslit("errorCode")).get<double>());
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

			AccountUuid referrer_uuid;

			const auto create_or_update_account = [&](const std::string &cur_login_name, unsigned cur_level){
				LOG_EMPERY_CENTER_DEBUG("Create or update account: cur_login_name = ", cur_login_name, ", cur_level = ", cur_level);
				auto account = AccountMap::get_by_login_name(PLATFORM_ID, cur_login_name);
				if(!account){
					const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
					const auto utc_now = Poseidon::get_utc_time();
					LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid, ", cur_login_name = ", cur_login_name);
					account = boost::make_shared<Account>(account_uuid, PLATFORM_ID, cur_login_name,
						referrer_uuid, cur_level, utc_now, cur_login_name);
					AccountMap::insert(account, ip); // XXX use real ip
				} else {
					account->set_promotion_level(cur_level);
				}
				referrer_uuid = account->get_account_uuid();
				return account;
			};

			for(auto it = referrers_array.rbegin(); it != referrers_array.rend(); ++it){
				auto &elem = it->get<Poseidon::JsonObject>();

				const auto &referrer_login_name = elem.at(sslit("loginName")).get<std::string>();
				const auto &referrer_level_str = elem.at(sslit("level")).get<std::string>();
				const unsigned referrer_level = g_level_config.at(referrer_level_str);

				create_or_update_account(referrer_login_name, referrer_level);
			}

			const auto level_str = response_object.at(sslit("level")).get<std::string>();
			const unsigned level = g_level_config.at(level_str);
			new_account = create_or_update_account(login_name, level);
		}
		return error_code;
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
				std::abort();
			}
		}
	};

	void send_verification_code(const std::string &login_name, const std::string &verification_code, std::uint64_t expiry_duration){
		PROFILE_ME;

		auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
		{
			const auto promise = Poseidon::DnsDaemon::async_lookup(sock_addr, g_server_host, g_server_port);
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}

		boost::shared_ptr<SimpleHttpClient> client;
		{
			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			client = boost::make_shared<SimpleHttpClient>(*sock_addr, g_server_use_ssl, promise);
			client->go_resident();

			Poseidon::OptionalMap get_params;
			get_params.set(sslit("loginName"), login_name);

			Poseidon::OptionalMap headers;
			headers.set(sslit("Host"), g_server_host);
			headers.set(sslit("Connection"), "Close");
			if(!g_server_auth.empty()){
				headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(g_server_auth));
			}
			client->send(g_server_path + "/queryAccountAttributes", std::move(get_params), std::move(headers));
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}
		std::istringstream iss(client->get_entity().dump());
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
			unsigned len = (unsigned)std::sprintf(str, "%.f", std::ceil(expiry_duration / 60000.0));
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

		sock_addr = boost::make_shared<Poseidon::SockAddr>();
		{
			const auto promise = Poseidon::DnsDaemon::async_lookup(sock_addr, g_sms_host, g_sms_port);
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}

		{
			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			client = boost::make_shared<SimpleHttpClient>(*sock_addr, g_sms_use_ssl, promise);
			client->go_resident();

			auto uri = g_sms_uri;
			pos = uri.find("$(phone)");
			if(pos != std::string::npos){
				uri.replace(pos, sizeof("$(phone)") - 1, Poseidon::Http::url_encode(phone_number));
			}
			pos = uri.find("$(message)");
			if(pos != std::string::npos){
				uri.replace(pos, sizeof("$(message)") - 1, Poseidon::Http::url_encode(message));
			}

			Poseidon::OptionalMap headers;
			headers.set(sslit("Host"), g_sms_host);
			headers.set(sslit("Connection"), "Close");
			if(!g_sms_auth.empty()){
				headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(g_sms_auth));
			}
			client->send(std::move(uri), { }, std::move(headers));
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}
	}

	int set_password(const std::string &login_name, const std::string &password){
		PROFILE_ME;

		const auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
		{
			const auto promise = Poseidon::DnsDaemon::async_lookup(sock_addr, g_server_host, g_server_port);
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}

		boost::shared_ptr<SimpleHttpClient> client;
		{
			const auto promise = boost::make_shared<Poseidon::JobPromise>();
			client = boost::make_shared<SimpleHttpClient>(*sock_addr, g_server_use_ssl, promise);
			client->go_resident();

			Poseidon::OptionalMap get_params;
			get_params.set(sslit("loginName"), login_name);
			get_params.set(sslit("password"),  password);

			Poseidon::OptionalMap headers;
			headers.set(sslit("Host"), g_server_host);
			headers.set(sslit("Connection"), "Close");
			if(!g_server_auth.empty()){
				headers.set(sslit("Authorization"), "Basic " + Poseidon::Http::base64_encode(g_server_auth));
			}
			client->send(g_server_path + "/setAccountAttributes", std::move(get_params), std::move(headers));
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}
		std::istringstream iss(client->get_entity().dump());
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
}

ACCOUNT_SERVLET("promotion/check_login", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &password   = params.at("password");
	const auto &token      = params.at("token");

	boost::shared_ptr<Account> account;
	const auto error_code = check_login_backtrace(account, login_name, password, session->get_remote_info().ip.get());
	if(error_code != Msg::ST_OK){
		return Response(error_code) <<login_name;
	}
	if(!account){
		DEBUG_THROW(Exception, sslit("Null account pointer"));
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<std::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("hasBeenActivated")]    = account->has_been_activated();
	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("promotion/renewal_token", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &old_token  = params.at("oldToken");
	const auto &token      = params.at("token");

	boost::shared_ptr<Account> account;
	const auto error_code = check_login_backtrace(account, login_name, std::string(), session->get_remote_info().ip.get());
	if((error_code != Msg::ST_OK) && (error_code != Msg::ERR_INVALID_PASSWORD)){
		return Response(error_code) <<login_name;
	}
	if(!account){
		DEBUG_THROW(Exception, sslit("Null account pointer"));
	}
	if(old_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	if(old_token != account->get_login_token()){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", account->get_login_token(), ", got ", old_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<std::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("promotion/regain", root, session, params){
	const auto &login_name = params.at("loginName");

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	const auto old_cooldown = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIFICATION_CODE_COOLDOWN);
	if(old_cooldown > utc_now){
		return Response(Msg::ERR_VERIFICATION_CODE_FLOOD) <<(old_cooldown - utc_now);
	}

	char str[64];
	unsigned len = (unsigned)std::sprintf(str, "%06u", (unsigned)(Poseidon::rand64() % 1000000));
	auto verification_code = std::string(str + len - 6, str + len);
	LOG_EMPERY_CENTER_DEBUG("Generated verification code: login_name = ", login_name, ", verification_code = ", verification_code);

	const auto expiry_duration = get_config<std::uint64_t>("promotion_verification_code_expiry_duration", 900000);
	auto expiry_time = boost::lexical_cast<std::string>(saturated_add(utc_now, expiry_duration));

	const auto cooldown_duration = get_config<std::uint64_t>("promotion_verification_code_cooldown_duration", 120000);
	auto cooldown = boost::lexical_cast<std::string>(saturated_add(utc_now, cooldown_duration));

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(3);
	modifiers.emplace(AccountAttributeIds::ID_VERIFICATION_CODE,             verification_code);
	modifiers.emplace(AccountAttributeIds::ID_VERIFICATION_CODE_EXPIRY_TIME, expiry_time);
	modifiers.emplace(AccountAttributeIds::ID_VERIFICATION_CODE_COOLDOWN,    cooldown);
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

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	const auto expiry_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_VERIFICATION_CODE_EXPIRY_TIME);
	if(expiry_time <= utc_now){
		return Response(Msg::ERR_VERIFICATION_CODE_EXPIRED);
	}
	const auto &code_expected = account->get_attribute(AccountAttributeIds::ID_VERIFICATION_CODE);
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
	const auto &login_name = params.at("loginName");
	const auto &code       = params.at("activationCode");

	const auto account = AccountMap::get_by_login_name(PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	if(account->has_been_activated()){
		return Response(Msg::ERR_ACCOUNT_ALREADY_ACTIVATED);
	}
	const auto activation_code = ActivationCodeMap::get(code);
	if(!activation_code || activation_code->is_virtually_removed()){
		return Response(Msg::ERR_ACTIVATION_CODE_DELETED);
	}

	account->activate();
	activation_code->set_used_by_account(account->get_account_uuid());

	return Response();
}

}
