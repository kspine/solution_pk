#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/dns_daemon.hpp>
#include <poseidon/singletons/dns_daemon.hpp>
#include <poseidon/sock_addr.hpp>
#include "../msg/err_account.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"

namespace EmperyCenter {

constexpr auto PLATFORM_ID = PlatformId(7800);

ACCOUNT_SERVLET("promotion/check_login", root, session, params){
//	const auto host = get_config<std::string>("promotion_server_host", "127.0.0.1");
//	const auto port = get_config<unsigned>   ("promotion_server_port", "6212");
//	const auto auth = get_config<std::string>("promotion_server_auth_user_pass");
//	const auto path = get_config<std::string>("promotion_server_path", "");

	return Response();
}

}
