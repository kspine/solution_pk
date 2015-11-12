#include "../precompiled.hpp"
#include "item_box_map.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "account_map.hpp"
#include "../item_box.hpp"
#include "../mysql/item.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

namespace {
	struct ItemBoxElement {
		AccountUuid accountUuid;
		boost::uint64_t unloadTime;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>> sink;
		mutable boost::shared_ptr<ItemBox> itemBox;

		ItemBoxElement(const AccountUuid &accountUuid_, boost::uint64_t unloadTime_)
			: accountUuid(accountUuid_), unloadTime(unloadTime_)
		{
		}
	};

	MULTI_INDEX_MAP(ItemBoxMapDelegator, ItemBoxElement,
		UNIQUE_MEMBER_INDEX(accountUuid)
		MULTI_MEMBER_INDEX(unloadTime)
	)

	boost::weak_ptr<ItemBoxMapDelegator> g_itemBoxMap;

	void gcTimerProc(boost::uint64_t now, boost::uint64_t period){
		PROFILE_ME;
		LOG_EMPERY_CENTER_DEBUG("Item box gc timer: now = ", now, ", period = ", period);

		const auto itemBoxMap = g_itemBoxMap.lock();
		if(!itemBoxMap){
			return;
		}

		for(;;){
			const auto it = itemBoxMap->begin<1>();
			if(it == itemBoxMap->end<1>()){
				break;
			}
			if(now < it->unloadTime){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() <= 1) && it->itemBox && it->itemBox.unique()){
				LOG_EMPERY_CENTER_INFO("Reclaiming item box: accountUuid = ", it->accountUuid);
				itemBoxMap->erase<1>(it);
			} else {
				itemBoxMap->setKey<1, 1>(it, now + 1000);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto itemBoxMap = boost::make_shared<ItemBoxMapDelegator>();
		g_itemBoxMap = itemBoxMap;
		handles.push(itemBoxMap);

		const auto gcInterval = getConfig<boost::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::registerTimer(0, gcInterval,
			std::bind(&gcTimerProc, std::placeholders::_2, std::placeholders::_3));
		handles.push(timer);
	}
}

boost::shared_ptr<ItemBox> ItemBoxMap::get(const AccountUuid &accountUuid){
	PROFILE_ME;

	const auto itemBoxMap = g_itemBoxMap.lock();
	if(!itemBoxMap){
		LOG_EMPERY_CENTER_WARNING("ItemBoxMap is not loaded.");
		DEBUG_THROW(Exception, sslit("ItemBoxMap is not loaded"));
	}

	auto it = itemBoxMap->find<0>(accountUuid);
	if(it == itemBoxMap->end<0>()){
		it = itemBoxMap->insert<0>(it, ItemBoxElement(accountUuid, 0));
	}
	if(!it->itemBox){
		LOG_EMPERY_CENTER_INFO("Loading item box: accountUuid = ", accountUuid);

		const auto now = Poseidon::getFastMonoClock();
		const auto gcInterval = getConfig<boost::uint64_t>("object_gc_interval", 300000);
		itemBoxMap->setKey<0, 1>(it, saturatedAdd(now, gcInterval));

		if(!it->promise){
			auto sink = boost::make_shared<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_Item` WHERE `accountUuid` = '" <<accountUuid <<"'";
			auto promise = Poseidon::MySqlDaemon::enqueueForBatchLoading(sink,
				&MySql::Center_Item::create, "Center_Item", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}

		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise);
		promise->checkAndRethrow();

		assert(it->sink);
		LOG_EMPERY_CENTER_DEBUG("Asynchronous MySQL query has completed: accountUuid = ", accountUuid, ", rows = ", it->sink->size());

		std::vector<boost::shared_ptr<MySql::Center_Item>> objs;
		objs.reserve(it->sink->size());
		for(auto sit = it->sink->begin(); sit != it->sink->end(); ++sit){
			auto obj = boost::dynamic_pointer_cast<MySql::Center_Item>(*sit);
			if(!obj){
				LOG_EMPERY_CENTER_ERROR("Unexpected dynamic MySQL object type: type = ", typeid(**sit).name());
				DEBUG_THROW(Exception, sslit("Unexpected dynamic MySQL object type"));
			}
			objs.emplace_back(std::move(obj));
		}
		auto itemBox = boost::make_shared<ItemBox>(accountUuid, objs);

		it->promise = { };
		it->sink    = { };
		it->itemBox = std::move(itemBox);
	}
	return it->itemBox;
}
boost::shared_ptr<ItemBox> ItemBoxMap::require(const AccountUuid &accountUuid){
	PROFILE_ME;

	auto ret = get(accountUuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("ItemBox not found: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("ItemBox not found"));
	}
	return ret;
}

}
