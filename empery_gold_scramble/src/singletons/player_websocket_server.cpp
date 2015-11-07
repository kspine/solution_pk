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
		PlayerWebSocketServer(const Poseidon::IpPort &bindAddr,
			const std::string &cert, const std::string &privateKey, std::string path)
			: Poseidon::TcpServerBase(bindAddr, cert.c_str(), privateKey.c_str())
			, m_path(std::move(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> onClientConnect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<PlayerSession>(std::move(client), m_path);
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = getConfig<std::string> ("player_websocket_server_bind", "0.0.0.0");
	auto port = getConfig<unsigned>    ("player_websocket_server_port", 15393);
	auto cert = getConfig<std::string> ("player_websocket_server_certificate");
	auto pkey = getConfig<std::string> ("player_websocket_server_private_key");
	auto path = getConfig<std::string> ("player_websocket_server_path", "/gold_scramble");

	const Poseidon::IpPort bindAddr(SharedNts(bind), port);
	LOG_EMPERY_GOLD_SCRAMBLE_INFO("Creating WebSocket player server on ", bindAddr);
	const auto server = boost::make_shared<PlayerWebSocketServer>(bindAddr, cert, pkey, std::move(path));
	Poseidon::EpollDaemon::registerServer(server);
	handles.push(server);
}

}
