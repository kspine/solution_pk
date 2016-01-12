#ifndef EMPERY_CENTER_FLAG_GUARD_HPP_
#define EMPERY_CENTER_FLAG_GUARD_HPP_

#include <poseidon/exception.hpp>
#include <cassert>

namespace EmperyCenter {

class FlagGuard {
private:
	bool &m_locked;

public:
	explicit FlagGuard(bool &locked)
		: m_locked(locked)
	{
		if(m_locked){
			DEBUG_THROW(Poseidon::Exception, Poseidon::SharedNts::view("Transaction locked"));
		}
		m_locked = true;
	}
	~FlagGuard(){
		if(!m_locked){
			assert(false);
		}
		m_locked = false;
	}

	FlagGuard(const FlagGuard &) = delete;
};

}

#endif
