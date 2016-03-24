#ifndef EMPERY_CENTER_STRATEGIC_RESOURCE_HPP_
#define EMPERY_CENTER_STRATEGIC_RESOURCE_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_StrategicResource;
}

class MapObject;
class PlayerSession;

class StrategicResource : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_StrategicResource> m_obj;

	// 非持久化数据。
	double m_harvest_remainder = 0;
	boost::weak_ptr<MapObject> m_last_harvester;

public:
	StrategicResource(Coord coord, ResourceId resource_id, std::uint64_t resource_amount,
		std::uint64_t created_time, std::uint64_t expiry_time);
	explicit StrategicResource(boost::shared_ptr<MySql::Center_StrategicResource> obj);
	~StrategicResource();

public:
	Coord get_coord() const;
	ResourceId get_resource_id() const;
	std::uint64_t get_resource_amount() const;
	std::uint64_t get_created_time() const;
	std::uint64_t get_expiry_time() const;

	bool has_been_deleted() const;
	void delete_from_game() noexcept;

	boost::shared_ptr<MapObject> get_last_harvester() const {
		return m_last_harvester.lock();
	}

	std::uint64_t harvest(const boost::shared_ptr<MapObject> &harvester, std::uint64_t duration, bool saturated);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_strategic_resource_with_player(const boost::shared_ptr<const StrategicResource> &strategic_resource,
	const boost::shared_ptr<PlayerSession> &session)
{
	strategic_resource->synchronize_with_player(session);
}
inline void synchronize_strategic_resource_with_player(const boost::shared_ptr<StrategicResource> &strategic_resource,
	const boost::shared_ptr<PlayerSession> &session)
{
	strategic_resource->synchronize_with_player(session);
}

}

#endif
