#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../account.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../../../empery_center/src/msg/err_account.hpp"
#include "../../../empery_center/src/msg/err_castle.hpp"

namespace EmperyController {

WORLD_SERVLET("query_castles", root, session, params){
	const auto &platform_id = boost::lexical_cast<PlatformId>(params.at("platformId"));
	const auto &login_name = params.at("loginName");

	const auto account = AccountMap::get_or_reload_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<login_name;
	}
	const auto account_uuid = account->get_account_uuid();

	std::vector<boost::shared_ptr<Castle>> castles;
	WorldMap::get_castles_by_owner(castles, account_uuid);

	Poseidon::JsonObject castles_obj;
	for(auto it = castles.begin(); it != castles.end(); ++it){
		const auto &castle = *it;

		Poseidon::JsonObject meta;
		meta[sslit("ownerUuid")]        = castle->get_owner_uuid().str();
		meta[sslit("parentObjectUuid")] = castle->get_parent_object_uuid().str();
		meta[sslit("name")]             = castle->get_name();
		meta[sslit("coordX")]           = castle->get_coord().x();
		meta[sslit("coordY")]           = castle->get_coord().y();
		meta[sslit("createdTime")]      = castle->get_created_time();
		meta[sslit("garrisoned")]       = castle->is_garrisoned();

		castles_obj[SharedNts(castle->get_map_object_uuid().str())] = std::move(meta);
	}
	root[sslit("castles")] = std::move(castles_obj);

	return Response();
}

}
