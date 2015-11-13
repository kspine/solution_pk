#ifndef EMPERY_PROMOTION_ACCOUNT_HTTP_SESSION_HPP_
#define EMPERY_PROMOTION_ACCOUNT_HTTP_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/http/authorization.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyPromotion {

class AccountHttpSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		Poseidon::JsonObject (const boost::shared_ptr<AccountHttpSession> &session, const Poseidon::OptionalMap &params)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(const std::string &uri);

private:
	const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_auth_info;
	const std::string m_prefix;

public:
	AccountHttpSession(Poseidon::UniqueFile socket,
		boost::shared_ptr<const Poseidon::Http::AuthInfo> auth_info, std::string prefix);
	~AccountHttpSession();

protected:
	boost::shared_ptr<Poseidon::Http::UpgradedSessionBase> predispatch_request(
		Poseidon::Http::RequestHeaders &request_headers, Poseidon::StreamBuffer &entity) override;

	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
