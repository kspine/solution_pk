#include "../precompiled.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	class DataHttpServer : public Poseidon::TcpServerBase {
	private:
		const std::string m_path;

	public:
		DataHttpServer(const Poseidon::IpPort &bindAddr, const std::string &cert, const std::string &privateKey,
			std::string path)
			: Poseidon::TcpServerBase(bindAddr, cert.c_str(), privateKey.c_str())
			, m_path(std::move(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> onClientConnect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<DataHttpSession>(std::move(client), m_path + '/');
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = getConfig<std::string> ("data_http_server_bind", "0.0.0.0");
	auto port = getConfig<unsigned>    ("data_http_server_port", 13218);
	auto cert = getConfig<std::string> ("data_http_server_certificate");
	auto pkey = getConfig<std::string> ("data_http_server_private_key");
	auto path = getConfig<std::string> ("data_http_server_path", "/empery/data");

	const auto bindAddr = Poseidon::IpPort(SharedNts(bind), port);
	LOG_EMPERY_CENTER_INFO("Creating data HTTP server on ", bindAddr, path);
	const auto server = boost::make_shared<DataHttpServer>(bindAddr, cert, pkey, std::move(path));
	Poseidon::EpollDaemon::registerServer(server);
	handles.push(server);
}

}
