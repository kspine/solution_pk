#include "../precompiled.hpp"
#include "../mmain.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../world_session.hpp"

namespace EmperyController {

namespace {
	class WorldHttpServer : public Poseidon::TcpServerBase {
	private:
		const std::string m_path;

	public:
		WorldHttpServer(const Poseidon::IpPort &bind_addr, const std::string &cert, const std::string &private_key,
			std::string path)
			: Poseidon::TcpServerBase(bind_addr, cert.c_str(), private_key.c_str())
			, m_path(std::move(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> on_client_connect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<WorldSession>(std::move(client), m_path + '/');
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = get_config<std::string>   ("world_http_server_bind", "0.0.0.0");
	auto port = get_config<unsigned>      ("world_http_server_port", 13210);
	auto cert = get_config<std::string>   ("world_http_server_certificate");
	auto pkey = get_config<std::string>   ("world_http_server_private_key");
	auto path = get_config<std::string>   ("world_http_server_path", "/empery/world");

	const auto bind_addr = Poseidon::IpPort(SharedNts(bind), port);
	LOG_EMPERY_CONTROLLER_INFO("Creating log HTTP server on ", bind_addr, path);
	const auto server = boost::make_shared<WorldHttpServer>(bind_addr, cert, pkey, std::move(path));
	Poseidon::EpollDaemon::register_server(server);
	handles.push(server);
}

}
