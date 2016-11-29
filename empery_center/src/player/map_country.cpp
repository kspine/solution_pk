#include "../precompiled.hpp"
#include "common.hpp"

#include "../msg/cs_map_country.hpp"
#include "../msg/sc_map_country.hpp"

#include "../singletons/world_map.hpp"

#include "../account_attribute_ids.hpp"

#include "../castle.hpp"

#include "../data/map_country.hpp"


namespace EmperyCenter{

  PLAYER_SERVLET(Msg::CS_GetMapCountryInfoReqMessage,account, session, req)
  {
     PROFILE_ME;

     const auto &map_name  = account->get_attribute(AccountAttributeIds::ID_MAP_COUNTRY);
     if(map_name.empty())
     {

     	const auto primary_castle =  WorldMap::get_primary_castle(account->get_account_uuid());
     	if(!primary_castle)
     	{
     	   LOG_EMPERY_CENTER_ERROR("……Primary Castle not found……");

     	   return Response(Msg::ST_OK);
     	}

        const auto scope = WorldMap::get_cluster_scope(primary_castle->get_coord());

        std::int64_t numerical_x = scope.left() / static_cast<std::int64_t>(scope.width());
        std::int64_t numerical_y = scope.bottom() / static_cast<std::int64_t>(scope.height());

        std::string map_id = boost::lexical_cast<std::string>(numerical_x) + ","+ boost::lexical_cast<std::string>(numerical_y);

        std::string map_names =  Data::MapCountry::get_map_name(map_id);

        //update account attributive 	
     	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
     	modifiers.reserve(1);
     	modifiers[AccountAttributeIds::ID_MAP_COUNTRY]  = map_names;
        account->set_attributes(std::move(modifiers));

        Msg::SC_GetMapCountryInfoReqMessage msg;
        msg.map_name = map_names;
        session->send(msg);

        return Response(Msg::ST_OK);
     }
     else
     {
        Msg::SC_GetMapCountryInfoReqMessage msg;
        msg.map_name = map_name;
        session->send(msg);

        return Response(Msg::ST_OK);
     }
  }
}
