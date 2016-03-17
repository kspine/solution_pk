#include "../precompiled.hpp"
#include "../mmain.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../log_session.hpp"

namespace EmperyCenterLog {

namespace {
	class LogCbppServer : public Poseidon::TcpServerBase {
	public:
		LogCbppServer(const Poseidon::IpPort &bind_addr, const std::string &cert, const std::string &private_key)
			: Poseidon::TcpServerBase(bind_addr, cert.c_str(), private_key.c_str())
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> on_client_connect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<LogSession>(std::move(client));
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = get_config<std::string>   ("log_cbpp_server_bind", "0.0.0.0");
	auto port = get_config<unsigned>      ("log_cbpp_server_port", 13214);
	auto cert = get_config<std::string>   ("log_cbpp_server_certificate");
	auto pkey = get_config<std::string>   ("log_cbpp_server_private_key");

	const auto bind_addr = Poseidon::IpPort(SharedNts(bind), port);
	LOG_EMPERY_CENTER_LOG_INFO("Creating log CBPP server on ", bind_addr);
	const auto server = boost::make_shared<LogCbppServer>(bind_addr, cert, pkey);
	Poseidon::EpollDaemon::register_server(server);
	handles.push(server);
}

}
