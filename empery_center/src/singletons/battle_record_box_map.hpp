#ifndef EMPERY_CENTER_SINGLETONS_BATTLE_RECORD_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_BATTLE_RECORD_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class BattleRecordBox;

struct BattleRecordBoxMap {
	static boost::shared_ptr<BattleRecordBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<BattleRecordBox> require(AccountUuid account_uuid);

private:
	BattleRecordBoxMap() = delete;
};

}

#endif
