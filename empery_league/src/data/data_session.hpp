#ifndef EMPERY_LEAGUE_DATA_SESSION_HPP_
#define EMPERY_LEAGUE_DATA_SESSION_HPP_

#include <poseidon/http/session.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyLeague {

class DataSession : public Poseidon::Http::Session {
public:
	struct SerializedData {
		std::string utf8_string;
		std::string md5_entry;
	};

public:
	static boost::shared_ptr<const SerializedData> create_servlet(const std::string &name, const Poseidon::JsonArray &data);
	static boost::shared_ptr<const SerializedData> get_servlet(const std::string &name);

private:
	const std::string m_prefix;

public:
	DataSession(Poseidon::UniqueFile socket, std::string prefix);
	~DataSession();

protected:
	void on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer entity) override;
};

}

#endif
