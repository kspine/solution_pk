#ifndef EMPERY_GOLD_SCRAMBLE_CBPP_RESPONSE_HPP_
#define EMPERY_GOLD_SCRAMBLE_CBPP_RESPONSE_HPP_

#include <poseidon/cbpp/status_codes.hpp>
#include <string>
#include <sstream>
#include <utility>

namespace EmperyGoldScramble {

class CbppResponse {
private:
	Poseidon::Cbpp::StatusCode m_statusCode;
	std::ostringstream m_message;

public:
	explicit CbppResponse(Poseidon::Cbpp::StatusCode statusCode = Poseidon::Cbpp::ST_OK)
		: m_statusCode(statusCode)
	{
	}

public:
	template<typename T>
	CbppResponse &operator<<(const T &rhs){
		m_message <<rhs;
		return *this;
	}
	template<typename T>
	operator std::pair<T, std::string>() const {
		return std::make_pair(m_statusCode, m_message.str());
	}
};

}

#endif
