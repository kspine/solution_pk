#include "../precompiled.hpp"
#include "promotion.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>

namespace EmperyPromotion {

namespace {
	MULTI_INDEX_MAP(PromotionMap, Data::Promotion,
		UNIQUE_MEMBER_INDEX(level)
		UNIQUE_MEMBER_INDEX(display_level)
	);

	const char DATA_FILE[] = "empery_promotion_levels.csv";

	boost::weak_ptr<const PromotionMap> g_map;
}

MODULE_RAII_PRIORITY(handles, 1000){
	LOG_EMPERY_PROMOTION_INFO("Loading promotion from: ", DATA_FILE);
	const auto map = boost::make_shared<PromotionMap>();

	Poseidon::CsvParser csv(DATA_FILE);
	while(csv.fetch_row()){
		Data::Promotion elem = { };

		csv.get(elem.level,                   "level");
		csv.get(elem.display_level,           "displayLevel");
		csv.get(elem.name,                    "name");
		csv.get(elem.tax_ratio,               "taxRatio");
		csv.get(elem.tax_extra,               "taxExtra");
		csv.get(elem.immediate_price,         "immediatePrice");
		csv.get(elem.immediate_discount,      "immediateDiscount");
		csv.get(elem.auto_upgrade_count,      "autoUpgradeCount");
		csv.get(elem.large_gift_box_price,    "largeGiftBoxPrice");
		csv.get(elem.shared_recharge_enabled, "sharedRechargeEnabled");

		if(!map->insert(std::move(elem)).second){
			LOG_EMPERY_PROMOTION_ERROR("Duplicate promotion element: level = ", elem.level);
			DEBUG_THROW(Exception, sslit("Duplicate promotion element"));
		}
	}

	LOG_EMPERY_PROMOTION_INFO("Done loading promotion");
	g_map = map;
	handles.push(map);
}

namespace Data {
	boost::shared_ptr<const Promotion> Promotion::get(std::uint64_t level){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return { };
		}

		auto it = map->upper_bound<0>(level);
		if(it == map->begin<0>()){
			return { };
		}
		--it;
		return boost::shared_ptr<const Promotion>(std::move(map), &*it);
	}
	boost::shared_ptr<const Promotion> Promotion::require(std::uint64_t level){
		auto ret = get(level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Promotion element not found"));
		}
		return ret;
	}

	boost::shared_ptr<const Promotion> Promotion::get_by_display_level(unsigned display_level){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return { };
		}

		auto it = map->find<1>(display_level);
		if(it == map->end<1>()){
			return { };
		}
		return boost::shared_ptr<const Promotion>(std::move(map), &*it);
	}
	boost::shared_ptr<const Promotion> Promotion::require_by_display_level(unsigned display_level){
		auto ret = get_by_display_level(display_level);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Promotion element not found"));
		}
		return ret;
	}

	void Promotion::get_all(std::vector<boost::shared_ptr<const Promotion>> &ret){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + map->size());
		for(auto it = map->begin<0>(); it != map->end<0>(); ++it){
			ret.emplace_back(std::move(map), &*it);
		}
	}

	boost::shared_ptr<const Promotion> Promotion::get_first(){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return { };
		}

		auto it = map->begin<0>();
		if(it == map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const Promotion>(std::move(map), &*it);
	}
	boost::shared_ptr<const Promotion> Promotion::get_next(std::uint64_t level){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return { };
		}

		auto it = map->upper_bound<0>(level);
		if(it == map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const Promotion>(std::move(map), &*it);
	}
}

}
