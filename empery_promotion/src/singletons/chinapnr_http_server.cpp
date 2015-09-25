#include "../precompiled.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../chinapnr_http_session.hpp"

namespace EmperyPromotion {

namespace {
	class ChinaPnRHttpServer : public Poseidon::TcpServerBase {
	private:
		const std::string m_path;

	public:
		ChinaPnRHttpServer(const Poseidon::IpPort &bindAddr, const std::string &cert, const std::string &privateKey, std::string path)
			: Poseidon::TcpServerBase(bindAddr, cert.c_str(), privateKey.c_str())
			, m_path(std::move(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> onClientConnect(Poseidon::UniqueFile client) const override {
			return boost::make_shared<ChinaPnRHttpSession>(std::move(client), m_path + '/');
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto bind = getConfig<std::string>("chinapnr_http_server_bind", "0.0.0.0");
	auto port = getConfig<unsigned>("chinapnr_http_server_port", 10315);
	auto cert = getConfig<std::string>("chinapnr_http_server_certificate");
	auto pkey = getConfig<std::string>("chinapnr_http_server_private_key");
	auto path = getConfig<std::string>("chinapnr_http_server_path", "/empery_promotion/chinapnr");

	const Poseidon::IpPort bindAddr(SharedNts(bind), port);
	LOG_EMPERY_PROMOTION_INFO("Creating ChinaPnR HTTP server on ", bindAddr, path);
	const auto server = boost::make_shared<ChinaPnRHttpServer>(bindAddr, cert, pkey, std::move(path));
	Poseidon::EpollDaemon::registerServer(server);
	handles.push(server);
}

}
