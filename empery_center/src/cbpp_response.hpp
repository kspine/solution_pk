#ifndef EMPERY_CENTER_CBPP_RESPONSE_HPP_
#define EMPERY_CENTER_CBPP_RESPONSE_HPP_

#include <poseidon/cbpp/status_codes.hpp>
#include <string>
#include <sstream>
#include <utility>

namespace EmperyCenter {

class CbppResponse {
private:
	Poseidon::Cbpp::StatusCode m_status_code;
	std::ostringstream m_message;

public:
	explicit CbppResponse(Poseidon::Cbpp::StatusCode status_code = Poseidon::Cbpp::ST_OK)
		: m_status_code(status_code)
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
		return std::make_pair(m_status_code, m_message.str());
	}
};

}

#endif
