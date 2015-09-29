#ifndef EMPERY_PROMOTION_SINGLETONS_CHINAPNR_SIGN_DAEMON_HPP_
#define EMPERY_PROMOTION_SINGLETONS_CHINAPNR_SIGN_DAEMON_HPP_

#include <string>

namespace EmperyPromotion {

struct ChinaPnrSignDaemon {
	static std::string sign(const std::string &merId, const std::string &serial, boost::uint64_t createdTime,
		boost::uint64_t amount, const std::string &retUrl, const std::string &usrMp, const std::string &bgRetUrl);
	static bool check(const std::string &cmdId, const std::string &merId, const std::string &respCode,
		const std::string &trxId, const std::string &ordAmt, const std::string &curCode, const std::string &pid,
		const std::string &ordId, const std::string &merPriv, const std::string &retType, const std::string &divDetails,
		const std::string &gateId, const std::string &chkValue);

private:
	ChinaPnrSignDaemon();
};

}

#endif
