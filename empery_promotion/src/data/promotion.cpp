#include "../precompiled.hpp"
#include "promotion.hpp"
#include <poseidon/multi_index_map.hpp>
#include "formats.hpp"
// #include "../resource_http_session.hpp"

namespace EmperyPromotion {

namespace {
	MULTI_INDEX_MAP(PromotionMap, Data::Promotion,
		UNIQUE_MEMBER_INDEX(price)
	);

	const char DATA_FILE[] = "empery_data/promotion.csv";

	boost::weak_ptr<const PromotionMap> g_map;
}

MODULE_RAII_PRIORITY(handles, 1000){
	LOG_EMPERY_PROMOTION_INFO("Loading promotion from: ", DATA_FILE);
	const auto map = boost::make_shared<PromotionMap>();

	Poseidon::CsvParser csv(DATA_FILE);
	while(csv.fetchRow()){
		Data::Promotion elem = { };

		csv.get(elem.price,             "price");
		csv.get(elem.level,             "level");
		csv.get(elem.name,              "name");
		csv.get(elem.taxRatio,          "taxRatio");
		csv.get(elem.taxExtra,          "taxExtra");
		csv.get(elem.immediatePrice,    "immediatePrice");
		csv.get(elem.immediateDiscount, "immediateDiscount");
		csv.get(elem.autoUpgradeCount,  "autoUpgradeCount");

		if(!map->insert(std::move(elem)).second){
			LOG_EMPERY_PROMOTION_ERROR("Duplicate promotion element: price = ", elem.price);
			DEBUG_THROW(Exception, sslit("Duplicate promotion element"));
		}
	}

	LOG_EMPERY_PROMOTION_INFO("Done loading promotion");
	g_map = map;
//	handles.push(ResourceHttpSession::createServlet("promotion", serializeCsv(csv, "price")));
	handles.push(map);
}

namespace Data {
	boost::shared_ptr<const Promotion> Promotion::get(boost::uint64_t price){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return { };
		}

		auto it = map->upperBound<0>(price);
		if(it == map->begin<0>()){
			return { };
		}
		--it;
		return boost::shared_ptr<const Promotion>(std::move(map), &*it);
	}
	boost::shared_ptr<const Promotion> Promotion::require(boost::uint64_t price){
		auto ret = get(price);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Promotion element not found"));
		}
		return ret;
	}

	boost::shared_ptr<const Promotion> Promotion::getNext(boost::uint64_t price){
		auto map = g_map.lock();
		if(!map){
			LOG_EMPERY_PROMOTION_ERROR("Promotion data has not been loaded.");
			return { };
		}

		auto it = map->upperBound<0>(price);
		if(it == map->end<0>()){
			return { };
		}
		return boost::shared_ptr<const Promotion>(std::move(map), &*it);
	}
}

}
