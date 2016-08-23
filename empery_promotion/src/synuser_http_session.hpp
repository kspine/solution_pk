#ifndef EMPERY_PROMOTION_SYNUSER_HTTP_SESSION_HPP_
#define EMPERY_PROMOTION_SYNUSER_HTTP_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyPromotion {

class SynuserHttpSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		Poseidon::JsonObject (const boost::shared_ptr<SynuserHttpSession> &session, Poseidon::OptionalMap params)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(const std::string &uri);

private:
	const std::string m_prefix;

public:
	SynuserHttpSession(Poseidon::UniqueFile socket, std::string prefix);
	~SynuserHttpSession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
