#ifndef EMPERY_CENTER_AUCTION_HTTP_SESSION_HPP_
#define EMPERY_CENTER_AUCTION_HTTP_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/http/authorization.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

class AuctionHttpSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		 std::pair<long, std::string> (Poseidon::JsonObject &, const boost::shared_ptr<AuctionHttpSession> &, Poseidon::OptionalMap)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(const std::string &uri);

private:
	const boost::shared_ptr<const Poseidon::Http::AuthInfo> m_auth_info;
	const std::string m_prefix;

public:
	AuctionHttpSession(Poseidon::UniqueFile socket,
		boost::shared_ptr<const Poseidon::Http::AuthInfo> auth_info, std::string prefix);
	~AuctionHttpSession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
