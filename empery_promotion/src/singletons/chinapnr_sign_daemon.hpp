#ifndef EMPERY_PROMOTION_SINGLETONS_CHINAPNR_SIGN_DAEMON_HPP_
#define EMPERY_PROMOTION_SINGLETONS_CHINAPNR_SIGN_DAEMON_HPP_

#include <string>

namespace EmperyPromotion {

struct ChinaPnrSignDaemon {
	static std::string sign(const std::string &merId, const std::string &serial,
		boost::uint64_t createdTime, const std::string &amount, const std::string &bgRetUrl, const std::string &retUrl);

private:
	ChinaPnrSignDaemon();
};

}

#endif
