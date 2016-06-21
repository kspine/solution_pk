#ifndef EMPERY_CENTER_DATA_ACTIVITY_HPP_
#define EMPERY_CENTER_DATA_ACTIVITY_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace EmperyCenter {

namespace Data {
	class Activity{
	public:
		static boost::shared_ptr<const Activity> get(std::uint64_t unique_id);
		static boost::shared_ptr<const Activity> require(std::uint64_t unique_id);
		static void get_all(std::vector<boost::shared_ptr<const Activity>> &ret);
	public:
		std::uint64_t unique_id;
		std::uint64_t available_since;
		std::uint64_t available_until;
	};

	class MapActivity{
	public:
		static boost::shared_ptr<const MapActivity> get(std::uint64_t unique_id);
		static boost::shared_ptr<const MapActivity> require(std::uint64_t unique_id);
		static void get_all(std::vector<boost::shared_ptr<const MapActivity>> &ret);
	public:
		std::uint64_t unique_id;
		unsigned      activity_type;
		std::uint64_t continued_time;
		boost::container::flat_map<std::uint64_t, std::vector<std::pair<std::uint64_t,std::uint64_t>>> rewards;
	};

	class ActivityAward{
	public:
		static bool get_activity_rank_award(std::uint64_t activity_id,const std::uint64_t rank,std::vector<std::pair<std::uint64_t,std::uint64_t>> &rewards);
	public:
		std::uint64_t unique_id;
		std::uint64_t activity_id;
		std::uint64_t rank_begin;
		std::uint64_t rank_end;
		std::vector<std::pair<std::uint64_t,std::uint64_t>> rewards;
	};
}

}

#endif
