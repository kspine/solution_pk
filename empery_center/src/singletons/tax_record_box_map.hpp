#ifndef EMPERY_CENTER_SINGLETONS_TAX_RECORD_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_TAX_RECORD_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class TaxRecordBox;

struct TaxRecordBoxMap {
	static boost::shared_ptr<TaxRecordBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<TaxRecordBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

private:
	TaxRecordBoxMap() = delete;
};

}

#endif
