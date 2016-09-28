#ifndef EMPERY_CENTER_LEGIONLOG_HPP_
#define EMPERY_CENTER_LEGIONLOG_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>

namespace EmperyCenter {


class LegionLog {
public:
	enum LEGION_MEMBER_LOG
	{
		LEGION_MEMBER_LOG_ADD = 0,
	};

public:
	LegionLog();
	~LegionLog();
	
};

}

#endif
