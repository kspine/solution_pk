#ifndef EMPERY_LEAGUE_LEAGUE_ATTRIBUTE_IDS_HPP_
#define EMPERY_LEAGUE_LEAGUE_ATTRIBUTE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyLeague {

namespace LeagueAttributeIds {

constexpr LeagueAttributeId

	ID_BEGIN                          (   0 ),
	ID_CUSTOM_PUBLIC_END              ( 100 ),
	ID_CUSTOM_END                     ( 200 ),
	ID_PUBLIC_END                     ( 300 ),
	ID_END                            ( 500 ),

	ID_LEVEL                          ( 101 ),
	ID_MONEY                          ( 102 ),
	ID_RANK                           ( 103 ),
	ID_CONTENT                        ( 104 ),
	ID_MEMBER_MAX                     ( 105 ),
	ID_ICON                           ( 106 ),
	ID_LANAGE                         ( 107 ),
	ID_AUTOJOIN                       ( 108 ),
	ID_LEADER                         ( 109 ),
	ID_ATTORNTIME                     ( 110 ),
	ID_ATTORNLEADER                   ( 111 );

}

}

#endif
