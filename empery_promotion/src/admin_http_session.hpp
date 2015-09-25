#ifndef EMPERY_PROMOTION_ADMIN_HTTP_SESSION_HPP_
#define EMPERY_PROMOTION_ADMIN_HTTP_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/http/authorization.hpp>

namespace EmperyPromotion {

class AdminHttpSession : public Poseidon::Http::Session {
public:
	typedef boost::function<
		void (const boost::shared_ptr<AdminHttpSession> &session, const Poseidon::OptionalMap &params)
		> ServletCallback;

public:
	static boost::shared_ptr<const ServletCallback> createServlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> getServlet(const std::string &uri);

private:
	const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_authInfo;
	const std::string m_prefix;

public:
	AdminHttpSession(Poseidon::UniqueFile socket,
		boost::shared_ptr<const Poseidon::Http::AuthInfo> authInfo, std::string prefix);
	~AdminHttpSession();

protected:
	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> predispatchRequest(
		Poseidon::Http::RequestHeaders &requestHeaders, Poseidon::StreamBuffer &entity) override;

	void onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer &entity) override;
};

}

#endif
