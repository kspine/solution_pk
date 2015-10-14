#include "../precompiled.hpp"
#include <poseidon/tcp_server_base.hpp>
#include <poseidon/http/authorization.hpp>
#include <poseidon/singletons/epoll_daemon.hpp>
#include "../account_http_session.hpp"

namespace TexasGateWestwalk {

namespace {
	class AccountHttpServer : public Poseidon::TcpServerBase {
	private:
		const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_authInfo;
		const std::string m_path;

	public:
		AccountHttpServer(const Poseidon::IpPort &bindAddr, const std::string &cert, const std::string &privateKey,
			std::vector<std::string> authInfo, std::string path)
			: Poseidon::TcpServerBase(bindAddr, cert.c_str(), privateKey.c_str())
			, m_authInfo(authInfo.empty() ? boost::shared_ptr<Poseidon::Http::AuthInfo>()
				: Poseidon::Http::createAuthInfo(STD_MOVE(authInfo))), m_path(STD_MOVE(path))
		{
		}

	protected:
		boost::shared_ptr<Poseidon::TcpSessionBase> onClientConnect(Poseidon::UniqueFile client) const OVERRIDE {
			return boost::make_shared<AccountHttpSession>(STD_MOVE(client), m_authInfo, m_path + '/');
		}
	};
}

MODULE_RAII_PRIORITY(handles, 9000){
	AUTO(bind, getConfig<std::string>("account_http_server_bind", "0.0.0.0"));
	AUTO(port, getConfig<unsigned>("account_http_server_port", 10311));
	AUTO(cert, getConfig<std::string>("account_http_server_certificate"));
	AUTO(pkey, getConfig<std::string>("account_http_server_private_key"));
	AUTO(auth, getConfigV<std::string>("account_http_server_auth_user_pass"));
	AUTO(path, getConfig<std::string>("account_http_server_path", "/texas_gate_westwalk/account"));

	const Poseidon::IpPort bindAddr(SharedNts(bind), port);
	LOG_TEXAS_GATE_WESTWALK_INFO("Creating account HTTP server on ", bindAddr, path);
	const AUTO(server, boost::make_shared<AccountHttpServer>(bindAddr, cert, pkey, STD_MOVE(auth), STD_MOVE(path)));
	Poseidon::EpollDaemon::registerServer(server);
	handles.push(server);
}

}
