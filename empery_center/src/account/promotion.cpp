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
#include "../msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../checked_arithmetic.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../activation_code.hpp"
#include "../../../empery_promotion/src/msg/err_account.hpp"

namespace EmperyCenter {

constexpr auto PROMOTION_PLATFORM_ID = PlatformId(8500);

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
	std::string g_server_auth    = "";
	std::string g_server_path    = "";

	boost::container::flat_map<std::string, unsigned> g_level_config;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		get_config(g_server_host,    "promotion_server_host");
		get_config(g_server_port,    "promotion_server_port");
		get_config(g_server_use_ssl, "account_http_server_use_ssl");
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
				error_code = Msg::ERR_NO_SUCH_ACCOUNT; // XXX
				break;
			}
		}
		const auto referrers_it = response_object.find(sslit("referrers"));
		if(referrers_it != response_object.end()){
			const auto &referrers_array = referrers_it->second.get<Poseidon::JsonArray>();

			AccountUuid referrer_uuid;

			const auto create_or_update_account = [&](const std::string &cur_login_name, unsigned cur_level){
				LOG_EMPERY_CENTER_DEBUG("Create or update account: cur_login_name = ", cur_login_name, ", cur_level = ", cur_level);
				auto account = AccountMap::get_by_login_name(PROMOTION_PLATFORM_ID, cur_login_name);
				if(!account){
					const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
					const auto utc_now = Poseidon::get_utc_time();
					LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid, ", cur_login_name = ", cur_login_name);
					account = boost::make_shared<Account>(account_uuid, PROMOTION_PLATFORM_ID, cur_login_name,
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

ACCOUNT_SERVLET("promotion/activate", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &code       = params.at("activationCode");

	const auto account = AccountMap::get_by_login_name(PROMOTION_PLATFORM_ID, login_name);
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
