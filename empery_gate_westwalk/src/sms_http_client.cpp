#include "precompiled.hpp"
#include "sms_http_client.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poseidon/sock_addr.hpp>
#include <poseidon/http/client.hpp>
#include <poseidon/singletons/timer_daemon.hpp>

namespace EmperyGateWestwalk {

namespace {
	std::string     g_http_host           = "localhost";
	unsigned        g_http_port           = 80;
	bool            g_http_use_ssl         = false;
	std::string     g_http_uri            = "/";
	std::string     g_http_account        = "";
	std::string     g_http_password       = "";

	Poseidon::SockAddr g_http_addr;

	MODULE_RAII_PRIORITY(/* handles */, 5000){
		get_config(g_http_host,           "sms_http_host");
		get_config(g_http_port,           "sms_http_port");
		get_config(g_http_use_ssl,         "sms_http_use_ssl");
		get_config(g_http_uri,            "sms_http_uri");
		get_config(g_http_account,        "sms_http_account");
		get_config(g_http_password,       "sms_http_password");

		LOG_EMPERY_GATE_WESTWALK_INFO("Looking up SMS server: ", g_http_host, ':', g_http_port);

		char port_str[64];
		std::sprintf(port_str, "%u", g_http_port);
		::addrinfo *res;
		const int err_code = ::getaddrinfo(g_http_host.c_str(), port_str, nullptr, &res);
		if(err_code != 0){
			LOG_EMPERY_GATE_WESTWALK_FATAL("Error looking up SMS server: err_code = ", err_code, ", message = ", ::gai_strerror(err_code));
			DEBUG_THROW(Exception, sslit("Error looking up SMS server"));
		}
		try {
			g_http_addr = Poseidon::SockAddr(res->ai_addr, res->ai_addrlen);
		} catch(...){
			::freeaddrinfo(res);
			throw;
		}
		::freeaddrinfo(res);
		LOG_EMPERY_GATE_WESTWALK_INFO("Result is ", Poseidon::get_ip_port_from_sock_addr(g_http_addr));
	}
}

class SmsHttpClient::HttpConnector : public Poseidon::Http::Client {
public:
	const boost::weak_ptr<SmsHttpClient> m_client;

	Poseidon::StreamBuffer m_contents;

public:
	HttpConnector(const Poseidon::SockAddr &sock_addr, bool use_ssl, boost::weak_ptr<SmsHttpClient> client)
		: Poseidon::Http::Client(sock_addr, use_ssl)
		, m_client(std::move(client))
	{
	}

protected:
	void on_sync_response_headers(Poseidon::Http::ResponseHeaders response_headers,
		std::string /* transfer_encoding */, boost::uint64_t /* content_length */) override
	{
		PROFILE_ME;
		LOG_EMPERY_GATE_WESTWALK_INFO("SMS server response: HTTP status = ", response_headers.status_code);

		const auto client = m_client.lock();
		if(!client){
			force_shutdown();
			return;
		}

		if(response_headers.status_code == Poseidon::Http::ST_OK){
			client->m_timer.reset();
		} else {
			LOG_EMPERY_GATE_WESTWALK_ERROR("SMS server has returned an error: status_code = ", response_headers.status_code);
		}
	}
	void on_sync_response_entity(boost::uint64_t /* entity_offset */, bool /* is_chunked */, Poseidon::StreamBuffer entity) override {
		m_contents.splice(entity);
	}
	void on_sync_response_end(boost::uint64_t content_length, bool /* is_chunked */, Poseidon::OptionalMap /* headers */) override {
		LOG_EMPERY_GATE_WESTWALK_INFO("SMS server response entity: content_length = ", content_length, ", contents = ", m_contents.dump());
	}
};

SmsHttpClient::SmsHttpClient(std::string phone, std::string password)
	: m_phone(std::move(phone)), m_password(std::move(password))
	, m_retry_count(0)
{
}
SmsHttpClient::~SmsHttpClient(){
}

void SmsHttpClient::timer_proc() noexcept {
	PROFILE_ME;

	if(!m_connector.expired()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Already connected...");
		return;
	}

	try {
		const auto max_retry_count = get_config<unsigned>("sms_max_retry_count", 6);
		if(m_retry_count > max_retry_count){
			LOG_EMPERY_GATE_WESTWALK_WARNING("Max retry count exceeded: retry_count = ", m_retry_count, ", max_retry_count = ", max_retry_count);
			DEBUG_THROW(Exception, sslit("Max retry count exceeded"));
		}
		++m_retry_count;

		auto password_expiry_duration = get_config<boost::uint64_t>("disposable_password_expiry_duration", 900000);
		auto product = get_config<std::string>("sms_product", "");

		auto text = get_config<std::string>("sms_text");

		static constexpr char S_PHONE    [] = "$(phone)";
		static constexpr char S_PASSWORD [] = "$(password)";
		static constexpr char S_MINUTES  [] = "$(minutes)";

		std::size_t pos;
		pos = text.find(S_PHONE);
		if(pos != std::string::npos){
			text.replace(pos, sizeof(S_PHONE) - 1, m_phone);
		}
		pos = text.find(S_PASSWORD);
		if(pos != std::string::npos){
			text.replace(pos, sizeof(S_PASSWORD) - 1, m_password);
		}
		pos = text.find(S_MINUTES);
		if(pos != std::string::npos){
			char str[64];
			auto len = (unsigned)std::sprintf(str, "%u", (unsigned)std::ceil(password_expiry_duration / 60000.0));
			text.replace(pos, sizeof(S_MINUTES) - 1, str, len);
		}

		Poseidon::Http::RequestHeaders headers;
		headers.verb = Poseidon::Http::V_GET;
		headers.uri = g_http_uri;
		headers.version = 10001;
		headers.get_params.set(sslit("account"),     g_http_account);
		headers.get_params.set(sslit("pswd"),        g_http_password);
		headers.get_params.set(sslit("mobile"),      m_phone);
		headers.get_params.set(sslit("msg"),         std::move(text));
		headers.get_params.set(sslit("needstatus"),  "false");
		headers.get_params.set(sslit("product"),     std::move(product));
		headers.headers.set(sslit("Host"), g_http_host);

		const auto connector = boost::make_shared<HttpConnector>(g_http_addr, g_http_use_ssl, shared_from_this());
		if(!connector->send(std::move(headers))){
			LOG_EMPERY_GATE_WESTWALK_WARNING("Failed to send data to SMS server");
			DEBUG_THROW(Exception, sslit("Failed to send data to SMS server"));
		}
		connector->go_resident();
		m_connector = connector;
	} catch(std::exception &e){
		LOG_EMPERY_GATE_WESTWALK_WARNING("std::exception thrown: what = ", e.what());
		m_timer.reset();
	}
}

void SmsHttpClient::commit(){
	const auto retry_interval = get_config<boost::uint64_t>("sms_retry_interval", 60000);
	m_timer = Poseidon::TimerDaemon::register_timer(0, retry_interval,
		// 故意绑定强引用。现在计时器成了该客户端的持有者，二者互相持有对方的强引用。
		// 在 timer_proc 中要及时打破这种循环引用，否则就发生内存泄漏。
		boost::bind(&SmsHttpClient::timer_proc, shared_from_this()));
}

}
