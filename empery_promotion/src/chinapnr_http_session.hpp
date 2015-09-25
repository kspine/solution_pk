#ifndef EMPERY_PROMOTION_CHINAPNR_HTTP_SESSION_HPP_
#define EMPERY_PROMOTION_CHINAPNR_HTTP_SESSION_HPP_

#include <poseidon/http/session.hpp>

namespace EmperyPromotion {

class ChinaPnRHttpSession : public Poseidon::Http::Session {
private:
	const std::string m_prefix;

public:
	ChinaPnRHttpSession(Poseidon::UniqueFile socket, std::string prefix);
	~ChinaPnRHttpSession();

private:
	void onSettleBill(const std::string &serial);

protected:
	void onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer &entity) override;
};

}

#endif
