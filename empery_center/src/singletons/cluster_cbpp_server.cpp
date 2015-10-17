#include "../precompiled.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../cluster_session.hpp"

namespace EmperyCenter {

namespace {
    class ClusterServer : public Poseidon::TcpServerBase {
    public:
        ClusterServer(const Poseidon::IpPort &bindAddr, const std::string &cert, const std::string &privateKey)
            : Poseidon::TcpServerBase(bindAddr, cert.c_str(), privateKey.c_str())
        {
        }

    protected:
        boost::shared_ptr<Poseidon::TcpSessionBase> onClientConnect(Poseidon::UniqueFile client) const override {
            return boost::make_shared<ClusterSession>(std::move(client));
        }
    };
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = getConfig<std::string> ("cluster_cbpp_server_bind", "0.0.0.0");
	auto port = getConfig<unsigned>    ("cluster_cbpp_server_port", 13217);
	auto cert = getConfig<std::string> ("cluster_cbpp_server_certificate");
	auto pkey = getConfig<std::string> ("cluster_cbpp_server_private_key");

	const Poseidon::IpPort bindAddr(SharedNts(bind), port);
	LOG_EMPERY_CENTER_INFO("Creating CBPP player server on ", bindAddr);
	const auto server = boost::make_shared<ClusterServer>(bindAddr, cert, pkey);
	Poseidon::EpollDaemon::registerServer(server);
	handles.push(server);
}

}





