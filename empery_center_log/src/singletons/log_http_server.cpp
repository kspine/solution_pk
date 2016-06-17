#include "../precompiled.hpp"
#include "../mmain.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/http/authorization.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../log_session.hpp"

namespace EmperyCenterLog {

namespace {
	class LogHttpServer : public Poseidon::TcpServerBase {
	private:
		const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_auth_info;
		const std::string m_path;

	public:
		LogHttpServer(const Poseidon::IpPort &bind_addr, const std::string &cert, const std::string &private_key,
			std::vector<std::string> auth_info, std::string path)
			: Poseidon::TcpServerBase(bind_addr, cert.c_str(), private_key.c_str())
			, m_auth_info(Poseidon::Http::create_auth_info(std::move(auth_info))), m_path(std::move(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> on_client_connect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<LogSession>(std::move(client), m_auth_info, m_path + '/');
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = get_config<std::string>   ("log_http_server_bind", "0.0.0.0");
	auto port = get_config<unsigned>      ("log_http_server_port", 13214);
	auto cert = get_config<std::string>   ("log_http_server_certificate");
	auto pkey = get_config<std::string>   ("log_http_server_private_key");
	auto auth = get_config_v<std::string> ("log_http_server_auth_user_pass");
	auto path = get_config<std::string>   ("log_http_server_path", "/empery/log");

	const auto bind_addr = Poseidon::IpPort(SharedNts(bind), port);
	LOG_EMPERY_CENTER_LOG_INFO("Creating log HTTP server on ", bind_addr, path);
	const auto server = boost::make_shared<LogHttpServer>(bind_addr, cert, pkey, std::move(auth), std::move(path));
	Poseidon::EpollDaemon::register_server(server);
	handles.push(server);
}

}
