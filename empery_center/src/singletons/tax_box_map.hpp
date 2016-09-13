#ifndef EMPERY_CENTER_SINGLETONS_TAX_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_TAX_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class TaxBox;

struct TaxBoxMap {
	static boost::shared_ptr<TaxBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<TaxBox> require(AccountUuid account_uuid);

private:
	TaxBoxMap() = delete;
};

}

#endif
