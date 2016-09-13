#ifndef EMPERY_LEAGUE_LEAGUE_SESSION_HPP_
#define EMPERY_LEAGUE_LEAGUE_SESSION_HPP_

#include <poseidon/cbpp/session.hpp>
#include <poseidon/fwd.hpp>
#include <poseidon/mutex.hpp>
#include <cstdint>
#include <boost/function.hpp>
#include <boost/container/flat_map.hpp>
#include <utility>

namespace EmperyLeague {

class LeagueSession : public Poseidon::Cbpp::Session {
public:
	using Result = std::pair<Poseidon::Cbpp::StatusCode, std::string>;

	using ServletCallback = boost::function<
		Result (const boost::shared_ptr<LeagueSession> &, Poseidon::StreamBuffer)>;

public:
	static boost::shared_ptr<const ServletCallback> create_servlet(std::uint64_t message_id, ServletCallback callback);
	static boost::shared_ptr<const ServletCallback> get_servlet(std::uint64_t message_id);

private:
	struct RequestElement {
		boost::shared_ptr<Result> result;
		boost::shared_ptr<Poseidon::JobPromise> promise;

		RequestElement(boost::shared_ptr<Result> result_, boost::shared_ptr<Poseidon::JobPromise> promise_)
			: result(std::move(result_)), promise(std::move(promise_))
		{
		}
	};

private:
	mutable Poseidon::Mutex m_request_mutex;
	boost::container::flat_multimap<Poseidon::Uuid, RequestElement> m_requests;

public:
	explicit LeagueSession(Poseidon::UniqueFile socket);
	~LeagueSession();

protected:
	void on_connect() override;
	void on_close(int err_code) noexcept override;

	bool on_low_level_data_message_end(std::uint64_t payload_size) override;

	void on_sync_data_message(std::uint16_t message_id, Poseidon::StreamBuffer payload) override;
	void on_sync_control_message(Poseidon::Cbpp::ControlCode control_code, std::int64_t vint_param, std::string string_param) override;

public:
	bool send(std::uint16_t message_id, Poseidon::StreamBuffer payload);
	void shutdown(const char *message) noexcept;
	void shutdown(int code, const char *message) noexcept;

	// 警告：不能在 servlet 中调用，否则会造成死锁。
	Result send_and_wait(std::uint16_t message_id, Poseidon::StreamBuffer payload);

	template<typename MsgT>
	bool send(const MsgT &msg){
		return send(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
	template<typename MsgT>
	Result send_and_wait(const MsgT &msg){
		return send_and_wait(MsgT::ID, Poseidon::StreamBuffer(msg));
	}
};

}

#endif
