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
	std::string     g_httpHost           = "localhost";
	unsigned        g_httpPort           = 80;
	bool            g_httpUseSsl         = false;
	std::string     g_httpUri            = "/";
	std::string     g_httpAccount        = "";
	std::string     g_httpPassword       = "";

	Poseidon::SockAddr g_httpAddr;

	MODULE_RAII_PRIORITY(/* handles */, 5000){
		getConfig(g_httpHost,           "sms_http_host");
		getConfig(g_httpPort,           "sms_http_port");
		getConfig(g_httpUseSsl,         "sms_http_use_ssl");
		getConfig(g_httpUri,            "sms_http_uri");
		getConfig(g_httpAccount,        "sms_http_account");
		getConfig(g_httpPassword,       "sms_http_password");

		LOG_EMPERY_GATE_WESTWALK_INFO("Looking up SMS server: ", g_httpHost, ':', g_httpPort);

		char portStr[64];
		std::sprintf(portStr, "%u", g_httpPort);
		::addrinfo *res;
		const int errCode = ::getaddrinfo(g_httpHost.c_str(), portStr, nullptr, &res);
		if(errCode != 0){
			LOG_EMPERY_GATE_WESTWALK_FATAL("Error looking up SMS server: errCode = ", errCode, ", message = ", ::gai_strerror(errCode));
			DEBUG_THROW(Exception, sslit("Error looking up SMS server"));
		}
		try {
			g_httpAddr = Poseidon::SockAddr(res->ai_addr, res->ai_addrlen);
		} catch(...){
			::freeaddrinfo(res);
			throw;
		}
		::freeaddrinfo(res);
		LOG_EMPERY_GATE_WESTWALK_INFO("Result is ", Poseidon::getIpPortFromSockAddr(g_httpAddr));
	}
}

class SmsHttpClient::HttpConnector : public Poseidon::Http::Client {
public:
	const boost::weak_ptr<SmsHttpClient> m_client;

	Poseidon::StreamBuffer m_contents;

public:
	HttpConnector(const Poseidon::SockAddr &sockAddr, bool useSsl, boost::weak_ptr<SmsHttpClient> client)
		: Poseidon::Http::Client(sockAddr, useSsl)
		, m_client(std::move(client))
	{
	}

protected:
	void onSyncResponseHeaders(Poseidon::Http::ResponseHeaders responseHeaders,
		std::string /* transferEncoding */, boost::uint64_t /* contentLength */) override
	{
		PROFILE_ME;
		LOG_EMPERY_GATE_WESTWALK_INFO("SMS server response: HTTP status = ", responseHeaders.statusCode);

		const auto client = m_client.lock();
		if(!client){
			forceShutdown();
			return;
		}

		if(responseHeaders.statusCode == Poseidon::Http::ST_OK){
			client->m_timer.reset();
		} else {
			LOG_EMPERY_GATE_WESTWALK_ERROR("SMS server has returned an error: statusCode = ", responseHeaders.statusCode);
		}
	}
	void onSyncResponseEntity(boost::uint64_t /* entityOffset */, bool /* isChunked */, Poseidon::StreamBuffer entity) override {
		m_contents.splice(entity);
	}
	void onSyncResponseEnd(boost::uint64_t contentLength, bool /* isChunked */, Poseidon::OptionalMap /* headers */) override {
		LOG_EMPERY_GATE_WESTWALK_INFO("SMS server response entity: contentLength = ", contentLength, ", contents = ", m_contents.dump());
	}
};

SmsHttpClient::SmsHttpClient(std::string phone, std::string password)
	: m_phone(std::move(phone)), m_password(std::move(password))
	, m_retryCount(0)
{
}
SmsHttpClient::~SmsHttpClient(){
}

void SmsHttpClient::timerProc() noexcept {
	PROFILE_ME;

	if(!m_connector.expired()){
		LOG_EMPERY_GATE_WESTWALK_DEBUG("Already connected...");
		return;
	}

	try {
		const auto maxRetryCount = getConfig<unsigned>("sms_max_retry_count", 6);
		if(m_retryCount > maxRetryCount){
			LOG_EMPERY_GATE_WESTWALK_WARNING("Max retry count exceeded: retryCount = ", m_retryCount, ", maxRetryCount = ", maxRetryCount);
			DEBUG_THROW(Exception, sslit("Max retry count exceeded"));
		}
		++m_retryCount;

		auto passwordExpiryDuration = getConfig<boost::uint64_t>("disposable_password_expiry_duration", 900000);
		auto product = getConfig<std::string>("sms_product", "");

		auto text = getConfig<std::string>("sms_text");

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
			auto len = (unsigned)std::sprintf(str, "%u", (unsigned)std::ceil(passwordExpiryDuration / 60000.0));
			text.replace(pos, sizeof(S_MINUTES) - 1, str, len);
		}

		Poseidon::Http::RequestHeaders headers;
		headers.verb = Poseidon::Http::V_GET;
		headers.uri = g_httpUri;
		headers.version = 10001;
		headers.getParams.set(sslit("account"),     g_httpAccount);
		headers.getParams.set(sslit("pswd"),        g_httpPassword);
		headers.getParams.set(sslit("mobile"),      m_phone);
		headers.getParams.set(sslit("msg"),         std::move(text));
		headers.getParams.set(sslit("needstatus"),  "false");
		headers.getParams.set(sslit("product"),     std::move(product));
		headers.headers.set(sslit("Host"), g_httpHost);

		const auto connector = boost::make_shared<HttpConnector>(g_httpAddr, g_httpUseSsl, shared_from_this());
		if(!connector->send(std::move(headers))){
			LOG_EMPERY_GATE_WESTWALK_WARNING("Failed to send data to SMS server");
			DEBUG_THROW(Exception, sslit("Failed to send data to SMS server"));
		}
		connector->goResident();
		m_connector = connector;
	} catch(std::exception &e){
		LOG_EMPERY_GATE_WESTWALK_WARNING("std::exception thrown: what = ", e.what());
		m_timer.reset();
	}
}

void SmsHttpClient::commit(){
	const auto retryInterval = getConfig<boost::uint64_t>("sms_retry_interval", 60000);
	m_timer = Poseidon::TimerDaemon::registerTimer(0, retryInterval,
		// 故意绑定强引用。现在计时器成了该客户端的持有者，二者互相持有对方的强引用。
		// 在 timerProc 中要及时打破这种循环引用，否则就发生内存泄漏。
		boost::bind(&SmsHttpClient::timerProc, shared_from_this()));
}

}
