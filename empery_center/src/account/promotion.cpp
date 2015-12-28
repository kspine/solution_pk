#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/dns_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/sock_addr.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/http/client.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/json.hpp>
#include "../msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../checked_arithmetic.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../activation_code.hpp"

namespace EmperyCenter {

constexpr auto PROMOTION_PLATFORM_ID = PlatformId(7800);

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

			if(!m_promise->is_satisfied()){
				try {
					m_promise->set_exception(boost::copy_exception(std::runtime_error("Lost connection to remote server")));
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
				}
			}

			Poseidon::Http::Client::on_close(err_code);
		}

		void on_sync_response_headers(Poseidon::Http::ResponseHeaders response_headers,
			std::string /* transfer_encoding */, boost::uint64_t /* content_length */) override
		{
			PROFILE_ME;

			m_headers = std::move(response_headers);
		}
		void on_sync_response_entity(boost::uint64_t /* entity_offset */, bool /* is_chunked */, Poseidon::StreamBuffer entity) override {
			PROFILE_ME;

			m_entity.splice(entity);
		}
		void on_sync_response_end(boost::uint64_t /* content_length */, bool /* is_chunked */, Poseidon::OptionalMap /* headers */) override {
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
		void send(std::string uri, Poseidon::OptionalMap get_params){
			PROFILE_ME;

			Poseidon::Http::RequestHeaders req_headers;
			req_headers.verb       = Poseidon::Http::V_GET;
			req_headers.uri        = std::move(uri);
			req_headers.version    = 10001;
			req_headers.get_params = std::move(get_params);
			req_headers.headers.set(sslit("Connection"), "Close");
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

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		get_config(g_server_host,    "promotion_server_host");
		get_config(g_server_port,    "promotion_server_port");
		get_config(g_server_use_ssl, "account_http_server_use_ssl");
		get_config(g_server_auth,    "promotion_server_auth_user_pass");
		get_config(g_server_path,    "promotion_server_path");
	}

	Poseidon::JsonObject get_json_from_promotion_server(const char *name, Poseidon::OptionalMap get_params){
		PROFILE_ME;

		const auto sock_addr = boost::make_shared<Poseidon::SockAddr>();
		{
			auto promise = Poseidon::DnsDaemon::async_lookup(sock_addr, g_server_host, g_server_port);
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}

		boost::shared_ptr<SimpleHttpClient> client;
		{
			auto promise = boost::make_shared<Poseidon::JobPromise>();
			client = boost::make_shared<SimpleHttpClient>(*sock_addr, g_server_use_ssl, std::move(promise));
			client->go_resident();
			client->send(g_server_path + "/" + name, std::move(get_params));
			Poseidon::JobDispatcher::yield(promise);
			promise->check_and_rethrow();
		}

		std::istringstream iss(client->get_entity().dump());
		return Poseidon::JsonParser::parse_object(iss);
	}
}

ACCOUNT_SERVLET("promotion/check_login", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &password   = params.at("password");
	const auto &token      = params.at("token");

	Poseidon::OptionalMap get_params;
	get_params.set(sslit("loginName"), login_name);
	get_params.set(sslit("password"),  password);
	get_params.set(sslit("ip"),        session->get_remote_info().ip.get());
	auto promotion_response_object = get_json_from_promotion_server("checkLogin", std::move(get_params));
	LOG_EMPERY_CENTER_DEBUG("Promotion server response: ", promotion_response_object.dump());
	auto error_code = static_cast<int>(promotion_response_object.at(sslit("errorCode")).get<double>());
	if(error_code != 0){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<login_name;
	}

	const auto utc_now = Poseidon::get_utc_time();

	AccountUuid referrer_uuid;
/*	std::deque<std::pair<std::string, unsigned>> referrers;
	referrers.emplace_back(login_name, 0);
	for(;;){
		const auto &cur_login_name = referrers.back().first;
		LOG_EMPERY_CENTER_DEBUG("Looking for referrer: cur_login_name = ", cur_login_name);

		get_params.clear();
		get_params.set(sslit("loginName"), cur_login_name);
		promotion_response_object = get_json_from_promotion_server("queryAccountAttributes", std::move(get_params));
		LOG_EMPERY_CENTER_DEBUG("Promotion server response: ", promotion_response_object.dump());
		error_code = static_cast<int>(promotion_response_object.at(sslit("errorCode")).get<double>());
		if(error_code != 0){
			break;
		}
		const auto &referrer_login_name = promotion_response_object.at(sslit("referrerLoginName")).get<std::string>();
		const auto &raw_level = promotion_response_object.at(sslit("level")).get<std::string>();
		
		const auto cur_referrer = AccountMap::get_by_login_name(PROMOTION_PLATFORM_ID, cur_login_name);
		if(cur_referrer){
			// 我们不在这里创建账号。如果第一个推荐人有账号，就假定向上的其余推荐人一定有。
			top_referrer_uuid = cur_referrer->get_account_uuid();
			break;
		}
		// 加入待创建列表。
		referrers.emplace_back(cur_login_name);
	}
	// 逆序创建这些账号。
	boost::shared_ptr<Account> account;
	for(auto it = referrers.rbegin(); it != referrers.rend(); ++it){
		const auto &cur_login_name = *it;
		const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
		LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid, ", cur_login_name = ", cur_login_name);
		account = boost::make_shared<Account>(account_uuid, PROMOTION_PLATFORM_ID, cur_login_name,
			top_referrer_uuid, 
	}

	assert(!referrers.empty());

	boost::
*/
	auto account = AccountMap::get_by_login_name(PROMOTION_PLATFORM_ID, login_name);
	if(!account){
		const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
		LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid, ", login_name = ", login_name);
		account = boost::make_shared<Account>(account_uuid, PROMOTION_PLATFORM_ID, login_name,
			referrer_uuid, 19, utc_now, login_name);
		AccountMap::insert(account, session->get_remote_info().ip.get());
	}
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	const auto token_expiry_duration = get_config<boost::uint64_t>("account_token_expiry_duration", 0);

	account->set_login_token(token, saturated_add(utc_now, token_expiry_duration));

	root[sslit("hasBeenActivated")]    = account->has_been_activated();
	root[sslit("tokenExpiryDuration")] = token_expiry_duration;

	return Response();
}

ACCOUNT_SERVLET("promotion/renewal_token", root, session, params){
	const auto &login_name = params.at("loginName");
	const auto &old_token  = params.at("oldToken");
	const auto &token      = params.at("token");

	const auto account = AccountMap::get_by_login_name(PROMOTION_PLATFORM_ID, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}
	if(old_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	if(old_token != account->get_login_token()){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", account->get_login_token(), ", got ", old_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}

	const auto token_expiry_duration = get_config<boost::uint64_t>("account_token_expiry_duration", 0);

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
