#ifndef EMPERY_CONTROLLER_WORLD_SESSION_HPP_
#define EMPERY_CONTROLLER_WORLD_SESSION_HPP_

#include <boost/function.hpp>
#include <poseidon/http/session.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyController {

class WorldSession : public Poseidon::Http::Session {
public:
	using ServletCallback = boost::function<
		 std::pair<long, std::string> (Poseidon::JsonObject &, const boost::shared_ptr<WorldSession> &, Poseidon::OptionalMap)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(const std::string &uri, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(const std::string &uri);

private:
	const std::string m_prefix;

public:
	WorldSession(Poseidon::UniqueFile socket, std::string prefix);
	~WorldSession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
