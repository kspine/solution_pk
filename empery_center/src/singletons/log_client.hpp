#ifndef EMPERY_CENTER_LOG_CLIENT_HPP_
#define EMPERY_CENTER_LOG_CLIENT_HPP_

#include <poseidon/cbpp/client.hpp>
#include <poseidon/fwd.hpp>
#include <cstdint>

namespace EmperyCenter {

class LogClient : public Poseidon::Cbpp::Client {
public:
	static boost::shared_ptr<LogClient> get();
	static boost::shared_ptr<LogClient> require();

private:
	LogClient(const Poseidon::SockAddr &sock_addr, bool use_ssl, std::uint64_t keep_alive_interval);

public:
	~LogClient();

protected:
	void on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload) override;
	void on_sync_error_message(std::uint16_t message_id, Poseidon::Cbpp::StatusCode status_code, std::string reason) override;

public:
	bool send(std::uint16_t message_id, Poseidon::StreamBuffer payload);
	void shutdown(const char *message) noexcept;
	void shutdown(int code, const char *message) noexcept;

	template<typename MsgT>
	bool send(const MsgT &msg){
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
};

}

#endif
