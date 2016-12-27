#ifndef EMPERY_DUNGEON_DEFENSE_MATRIX_HPP_
#define EMPERY_DUNGEON_DEFENSE_MATRIX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyDungeon {
namespace {

	constexpr DungeonBuffTypeId
	ID_BUFF_DEFENSE_MATRIX                 (6407001);//防御矩阵
}


class DungeonClient;
class Dungeon;
class DungeonObject;
class DefenseMatrix : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	DefenseMatrix(boost::weak_ptr<Dungeon> dungeon,std::vector<std::string> tags,std::uint64_t next_change_time,std::uint64_t interval);
	~DefenseMatrix();
public:
	void change_defense_matrix(std::uint64_t utc_now);
	std::uint64_t get_next_change_time(){
		return m_next_change_time;
	}
	bool is_ignore_die();
public:
	boost::weak_ptr<Dungeon> m_owner_dungeon;
	std::vector<std::string> m_defense_matrix;
	std::uint64_t            m_next_change_time;
	std::uint64_t            m_interval;
	DungeonObjectUuid        m_ignore_object_uuid;

};

}

#endif
