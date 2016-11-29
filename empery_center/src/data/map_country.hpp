#ifndef EMPERY_CENTER_DATA_MAP_COUNTRY_IN_HPP_
#define EMPERY_CENTER_DATA_MAP_COUNTRY_IN_HPP_


#include "common.hpp"
#include <boost/container/flat_map.hpp>

namespace EmperyCenter
{
 namespace Data
 {
   class MapCountry
   {
      public:
              static boost::shared_ptr<const MapCountry> get(std::string map_id);
              static boost::shared_ptr<const MapCountry> require(std::string map_id);
              static std::string get_map_name(std::string map_id);
      public:
              std::string map_id;
              std::string map_name;
   };
 }
}
#endif