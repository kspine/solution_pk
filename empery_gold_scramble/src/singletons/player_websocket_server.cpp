#include "../precompiled.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../player_session.hpp"

namespace EmperyGoldScramble {

namespace {
	class PlayerWebSocketServer : public Poseidon::TcpServerBase {
	private:
		const std::string m_path;

	public:
		PlayerWebSocketServer(const Poseidon::IpPort &bind_addr,
			const std::string &cert, const std::string &private_key, std::string path)
			: Poseidon::TcpServerBase(bind_addr, cert.c_str(), private_key.c_str())
			, m_path(std::move(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> on_client_connect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<PlayerSession>(std::move(client), m_path);
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = get_config<std::string> ("player_websocket_server_bind", "0.0.0.0");
	auto port = get_config<unsigned>    ("player_websocket_server_port", 15393);
	auto cert = get_config<std::string> ("player_websocket_server_certificate");
	auto pkey = get_config<std::string> ("player_websocket_server_private_key");
	auto path = get_config<std::string> ("player_websocket_server_path", "/gold_scramble");

	const Poseidon::IpPort bind_addr(SharedNts(bind), port);
	LOG_EMPERY_GOLD_SCRAMBLE_INFO("Creating WebSocket player server on ", bind_addr);
	const auto server = boost::make_shared<PlayerWebSocketServer>(bind_addr, cert, pkey, std::move(path));
	Poseidon::EpollDaemon::register_server(server);
	handles.push(server);
}

}
