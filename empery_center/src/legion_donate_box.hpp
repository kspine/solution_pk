#ifndef EMPERY_CENTER_LEGION_DONATE_BOX_HPP_
#define EMPERY_CENTER_LEGION_DONATE_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {
	namespace MySql {
		class Center_LegionDonate;
	}

	class PlayerSession;

	class LegionDonateBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
	public:

		struct DonateInfo {
			AccountUuid   account_uuid;
			std::uint64_t day_donate;
			std::uint64_t week_donate;
			std::uint64_t total_donate;
			std::uint64_t last_update_time;
		};

	private:
		const LegionUuid m_legion_uuid;

		boost::shared_ptr<MySql::Center_LegionDonate> m_stamps;

		boost::container::flat_map<AccountUuid,
			boost::shared_ptr<MySql::Center_LegionDonate>> m_donations;

	public:
		LegionDonateBox(LegionUuid legion_uuid,
			const std::vector<boost::shared_ptr<MySql::Center_LegionDonate>> &contributions);
		~LegionDonateBox();

	public:

		LegionUuid get_legion_uuid() const {
			return m_legion_uuid;
		}

		DonateInfo get(AccountUuid   account_uuid) const;
		void get_all(std::vector<DonateInfo> &ret) const;
		void update(AccountUuid account_uuid,std::uint64_t delta);
		void reset(std::uint64_t now) noexcept;
		void reset_day_donate(std::uint64_t now) noexcept;
		void reset_week_donate(std::uint64_t now) noexcept;
		void reset_account_donate(AccountUuid account_uuid) noexcept;
	};
}

#endif