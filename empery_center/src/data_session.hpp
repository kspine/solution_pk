#ifndef EMPERY_CENTER_DATA_SESSION_HPP_
#define EMPERY_CENTER_DATA_SESSION_HPP_

#include <poseidon/http/session.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

class DataSession : public Poseidon::Http::Session {
public:
	struct SerializedData {
		std::string utf8String;
		std::string md5Entry;
	};

public:
	static boost::shared_ptr<const SerializedData> createServlet(const std::string &name, const Poseidon::JsonArray &data);
	static boost::shared_ptr<const SerializedData> getServlet(const std::string &name);

private:
	const std::string m_prefix;

public:
	DataSession(Poseidon::UniqueFile socket, std::string prefix);
	~DataSession();

protected:
	void onSyncRequest(Poseidon::Http::RequestHeaders requestHeaders, Poseidon::StreamBuffer entity) override;
};

}

#endif
