#ifndef EMPERY_PROMOTION_ADMIN_HTTP_SESSION_HPP_
#define EMPERY_PROMOTION_ADMIN_HTTP_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/http/authorization.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyPromotion {

class AdminHttpSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		Poseidon::JsonObject (const boost::shared_ptr<AdminHttpSession> &session, Poseidon::OptionalMap params)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(const std::string &uri);

private:
	const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_auth_info;
	const std::string m_prefix;

public:
	AdminHttpSession(Poseidon::UniqueFile socket,
		boost::shared_ptr<const Poseidon::Http::AuthInfo> auth_info, std::string prefix);
	~AdminHttpSession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
