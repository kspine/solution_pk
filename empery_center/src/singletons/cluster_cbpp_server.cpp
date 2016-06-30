#include "../precompiled.hpp"
#include "../mmain.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../cluster_session.hpp"

namespace EmperyCenter {

namespace {
	class ClusterServer : public Poseidon::TcpServerBase {
	public:
		ClusterServer(const Poseidon::IpPort &bind_addr, const std::string &cert, const std::string &private_key)
			: Poseidon::TcpServerBase(bind_addr, cert.c_str(), private_key.c_str())
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> on_client_connect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<ClusterSession>(std::move(client));
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = get_config<std::string> ("cluster_cbpp_server_bind", "0.0.0.0");
	auto port = get_config<unsigned>    ("cluster_cbpp_server_port", 13227);
	auto cert = get_config<std::string> ("cluster_cbpp_server_certificate");
	auto pkey = get_config<std::string> ("cluster_cbpp_server_private_key");

	const Poseidon::IpPort bind_addr(SharedNts(bind), port);
	LOG_EMPERY_CENTER_INFO("Creating cluster CBPP server on ", bind_addr);
	const auto server = boost::make_shared<ClusterServer>(bind_addr, cert, pkey);
	Poseidon::EpollDaemon::register_server(server);
	handles.push(server);
}

}
