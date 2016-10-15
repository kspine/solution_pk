#ifndef EMPERY_CENTER_ACTIVITY_SESSION_HPP_
#define EMPERY_CENTER_ACTIVITY_SESSION_HPP_

#include <poseidon/http/session.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

class ActivitySession : public Poseidon::Http::Session {
public:
	struct SerializedData {
		std::string utf8_string;
		std::string md5_entry;
	};

public:
	static void create_servlet(const std::string &name, const Poseidon::JsonArray &data);
	static boost::shared_ptr<const SerializedData> get_servlet(const std::string &name);

private:
	const std::string m_prefix;

public:
	ActivitySession(Poseidon::UniqueFile socket, std::string prefix);
	~ActivitySession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
