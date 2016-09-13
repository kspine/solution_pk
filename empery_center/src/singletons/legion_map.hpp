#ifndef EMPERY_CENTER_SINGLETONS_LEGION_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"
#include "../legion.hpp"

namespace EmperyCenter {

//class Legion;
class PlayerSession;
class ItemBox;

struct LegionMap {
	static bool is_holding_controller_token(LegionUuid account_uuid);
	static void require_controller_token(LegionUuid account_uuid);

	static boost::shared_ptr<Legion> get(LegionUuid account_uuid);
	static boost::shared_ptr<Legion> require(LegionUuid account_uuid);
	static boost::shared_ptr<Legion> get_or_reload(LegionUuid account_uuid);
	static boost::shared_ptr<Legion> forced_reload(LegionUuid account_uuid);
	static boost::shared_ptr<Legion> get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name);
	static boost::shared_ptr<Legion> forced_reload_by_login_name(PlatformId platform_id, const std::string &login_name);

	static std::uint64_t get_count();
	static void get_all(std::vector<boost::shared_ptr<Legion>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_banned(std::vector<boost::shared_ptr<Legion>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_quieted(std::vector<boost::shared_ptr<Legion>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_by_nick(std::vector<boost::shared_ptr<Legion>> &ret, const std::string &nick);
	static void get_by_referrer(std::vector<boost::shared_ptr<Legion>> &ret, AccountUuid referrer_uuid);

	static boost::shared_ptr<Legion> get_by_account_uuid(AccountUuid account_uuid);
	static void insert(const boost::shared_ptr<Legion> &account, const std::string &remote_ip);
	static void update(const boost::shared_ptr<Legion> &account, bool throws_if_not_exists = true);
	static void deletelegion(LegionUuid legion_uuid);

private:
	LegionMap() = delete;
};

}

#endif
