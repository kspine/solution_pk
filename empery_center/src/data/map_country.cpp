#include "../precompiled.hpp"
#include "map_country.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
  namespace
  {
    MULTI_INDEX_MAP(MapCountryContainer, Data::MapCountry,
    UNIQUE_MEMBER_INDEX(map_id))

    boost::weak_ptr<const MapCountryContainer> g_map_country_container;
    const char COUNTRY_ID_FILE[] = "country_id";

    MODULE_RAII_PRIORITY(handles, 1000)
    {
      auto csv = Data::sync_load_data(COUNTRY_ID_FILE);
      const auto map_country_container = boost::make_shared<MapCountryContainer>();
      while (csv.fetch_row())
      {
        Data::MapCountry elem = {};

        csv.get(elem.map_id, "map_id");
        csv.get(elem.map_name, "map_name");


        LOG_EMPERY_CENTER_ERROR("map_country: map_id = ", elem.map_id);
        LOG_EMPERY_CENTER_ERROR("map_country: map_name = ", elem.map_name);
        if(!map_country_container->insert(std::move(elem)).second)
        {
           LOG_EMPERY_CENTER_ERROR("Duplicate map_country_container: map_id = ", elem.map_id);
           DEBUG_THROW(Exception, sslit("Duplicate map_country_container"));
        }
      }
      g_map_country_container = map_country_container;
      handles.push(map_country_container);
      auto servlet = DataSession::create_servlet(COUNTRY_ID_FILE, Data::encode_csv_as_json(csv, "map_id"));
      handles.push(std::move(servlet));
    }
  }

  namespace Data
  {
    boost::shared_ptr<const MapCountry> MapCountry::get(std::string map_id)
    {
      PROFILE_ME;
      const  auto map_country_container = g_map_country_container.lock();
      if(!map_country_container)
      {
        LOG_EMPERY_CENTER_WARNING("map_country has not been loaded.");
        return {};
      }
      const auto it = map_country_container->find<0>(map_id);
      if(it == map_country_container->end<0>())
      {
        LOG_EMPERY_CENTER_TRACE("map_country not found: map_id = ", map_id);
        return {};
      }
      return boost::shared_ptr<const MapCountry>(map_country_container,&*it);
    }

    boost::shared_ptr<const MapCountry> MapCountry::require(std::string map_id)
    {
      PROFILE_ME;
      auto ret = get(map_id);
      if(!ret)
      {
        LOG_EMPERY_CENTER_WARNING("map_country not found: map_id = ", map_id);
        DEBUG_THROW(Exception, sslit("map_country not found"));
        return {};
      }
      return ret;
    }

    std::string MapCountry::get_map_name(std::string map_id)
    {
      PROFILE_ME;
      auto ret = get(map_id);
      if(!ret)
      {
        LOG_EMPERY_CENTER_WARNING("map_country not found: map_id = ", map_id);
        DEBUG_THROW(Exception, sslit("map_country not found"));
        return "";
      }
      return ret->map_name;
    }
  }
}