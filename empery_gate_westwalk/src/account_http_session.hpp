#ifndef TEXAS_GATE_WESTWALK_ACCOUNT_HTTP_SESSION_HPP_
#define TEXAS_GATE_WESTWALK_ACCOUNT_HTTP_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/http/authorization.hpp>

namespace TexasGateWestwalk {

class AccountHttpSession : public Poseidon::Http::Session {
public:
	typedef boost::function<
		void (const boost::shared_ptr<AccountHttpSession> &session, const Poseidon::OptionalMap &params)
		> ServletCallback;

public:
	static boost::shared_ptr<const ServletCallback> createServlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> getServlet(const std::string &uri);

private:
	const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_authInfo;
	const std::string m_prefix;

public:
	AccountHttpSession(Poseidon::UniqueFile socket,
		boost::shared_ptr<const Poseidon::Http::AuthInfo> authInfo, std::string prefix);
	~AccountHttpSession();

protected:
	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> predispatchRequest(
		Poseidon::Http::RequestHeaders &requestHeaders, Poseidon::StreamBuffer &entity) OVERRIDE;

	void onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer &entity) OVERRIDE;
};

}

#endif
