#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("getPyramid", session, params){
	const auto &login_name = params.at("loginName");
	const auto &max_depth_str = params.get("maxDepth");
	const auto &view_performance_str = params.get("viewPerformance");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}

	const auto pyramid_depth_limit = get_config<unsigned>("pyramid_depth_limit", 32);

	Poseidon::JsonArray members;
	std::deque<std::pair<AccountId, Poseidon::JsonArray *>> this_level, next_level;
	this_level.emplace_back(info.account_id, &members);
	unsigned max_depth = 1;
	if(!max_depth_str.empty()){
		max_depth = boost::lexical_cast<unsigned>(max_depth_str);
	}
	if(max_depth > pyramid_depth_limit){
		max_depth = pyramid_depth_limit;
	}
	for(unsigned depth = 0; depth < max_depth; ++depth){
		if(this_level.empty()){
			break;
		}
		do {
			const auto current_account_id = this_level.front().first;
			const auto current_array_dest = this_level.front().second;
			this_level.pop_front();

			std::vector<AccountMap::AccountInfo> member_infos;
			AccountMap::get_by_referrer_id(member_infos, current_account_id);
			for(auto it = member_infos.begin(); it != member_infos.end(); ++it){
				LOG_EMPERY_PROMOTION_DEBUG("> Depth ", depth, ", account_id = ", it->account_id,
					", level = ", it->level, ", max_level = ", it->max_level, ", subordinate_count = ", it->subordinate_count);

				current_array_dest->emplace_back(Poseidon::JsonObject());
				auto &member = current_array_dest->back().get<Poseidon::JsonObject>();

				member[sslit("loginName")]       = std::move(it->login_name);
				member[sslit("nick")]            = std::move(it->nick);
				member[sslit("level")]           = boost::lexical_cast<std::string>(it->level);
				member[sslit("maxSubordLevel")]  = it->max_level;
				member[sslit("subordCount")]     = it->subordinate_count;
				if(!view_performance_str.empty()){
					member[sslit("performance")] = it->performance;
				}
				member[sslit("members")]         = Poseidon::JsonArray();

				next_level.emplace_back(it->account_id, &member[sslit("members")].get<Poseidon::JsonArray>());
			}
		} while(!this_level.empty());

		this_level.swap(next_level);
		next_level.clear();
	}

	ret[sslit("loginName")]       = std::move(info.login_name);
	ret[sslit("nick")]            = std::move(info.nick);
	ret[sslit("level")]           = boost::lexical_cast<std::string>(info.level);
	ret[sslit("maxSubordLevel")]  = info.max_level;
	ret[sslit("subordCount")]     = info.subordinate_count;
	if(!view_performance_str.empty()){
		ret[sslit("performance")] = info.performance;
	}
	ret[sslit("members")]         = std::move(members);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
