#ifndef EMPERY_CENTER_LOG_LOG_SESSION_HPP_
#define EMPERY_CENTER_LOG_LOG_SESSION_HPP_

#include <poseidon/cbpp/session.hpp>
#include <poseidon/fwd.hpp>
#include <cstdint>
#include <boost/function.hpp>
#include <utility>

namespace EmperyCenterLog {

class LogSession : public Poseidon::Cbpp::Session {
public:
	using ServletCallback = boost::function<
		void (const boost::shared_ptr<LogSession> &, Poseidon::StreamBuffer)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(std::uint16_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(std::uint16_t message_id);

public:
	explicit LogSession(Poseidon::UniqueFile socket);
	~LogSession();

protected:
	void on_connect() override;

	void on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload) override;
	void on_sync_control_message(Poseidon::Cbpp::ControlCode control_code, std::int64_t vint_param, std::string string_param) override;

public:
	void shutdown(const char *message) noexcept;
	void shutdown(int code, const char *message) noexcept;
};

}

#endif
