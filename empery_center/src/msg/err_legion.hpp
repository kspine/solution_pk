#ifndef EMPERY_CENTER_MSG_ERR_LEGION_HPP_
#define EMPERY_CENTER_MSG_ERR_LEGION_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_ACCOUNT_HAVE_LEGION                 			= 73000,	  	// 已经有军团
		ERR_LEGION_CREATE_NOTENOUGH_MONEY               	= 73001,		// 创建资源不足
		ERR_LEGION_CREATE_HOMONYM              				= 73002,		// 军团名已存在
		ERR_LEGION_NOT_JOIN              					= 73003,		// 没有加入军团
		ERR_LEGION_CANNOT_FIND             					= 73004,		// 没有找到军团
		ERR_LEGION_MEMBER_FULL            					= 73005,		// 军团人数已满
		ERR_LEGION_ALREADY_APPLY            				= 73006,		// 已经申请过同军团
		ERR_LEGION_NO_POWER            						= 73007,		// 没有相应权限
		ERR_LEGION_CANNOT_FIND_DATA            				= 73008,		// 数据不存在
		ERR_LEGION_CANNOT_FIND_ACCOUNT            			= 73009,		// 玩家不存在
		ERR_LEGION_QUIT_ERROR_LEGIONLEADER           		= 73010,		// 军团长需要先转让军团长
		ERR_LEGION_INVITED           						= 73011,		// 已经邀请过
		ERR_LEGION_NOT_IN_SAME_LEGION						= 73012,		// 不属于同一军团
		ERR_LEGION_ATTORN_IN_WAITTIME						= 73013,		// 已经在转让等待中了
		ERR_LEGION_TARGET_IS_OWN						    = 73014,		// 操作对象是目标自己
		ERR_LEGION_HAS_OTHER_MEMBERS						= 73015,		// 军团中还有其他成员
		ERR_LEGION_NOTFIND_INVITE_INFO						= 73016,		// 没有找到邀请信息
		ERR_LEGION_KICK_IN_WAITTIME							= 73017,		// 已经在踢出等待时间中
		ERR_LEGION_LEVEL_MEMBERS_LIMIT						= 73018,		// 当前等级的人数已达上限
		ERR_LEGION_TARGET_NO_POWER							= 73019,		// 目标无对应权限
		ERR_LEGION_LEVEL_CANNOT_SAME						= 73020,		// 不能调整到和自己同等级
		ERR_LEGION_MAINCITY_LOARD_LEVEL_LIMIT				= 73021,		// 主城领主府等级不够
		ERR_LEGION_CONFIG_CANNOT_FIND						= 73022,		// 没找到配置文件中对应的数据
		ERR_LEGION_APPLY_COUNT_LIMIT						= 73023,		// 当前可申请军团数量已达上限
		ERR_LEGION_BAN_CHAT									= 73024,		// 已经被禁言
		ERR_LEGION_DONATE_LACK								= 73025,		// 捐献时资源不足
		ERR_LEGION_STORE_CANNOT_FIND_ITEM					= 73026,		// 兑换军团商店道具时未找到道具
		ERR_LEGION_STORE_LOCK								= 73027,		// 兑换条件不满足，还未解锁
		ERR_LEGION_STORE_CONSUMED_LACK						= 73028,		// 兑换时所需消耗不足
		ERR_LEGION_STORE_LIMIT_PURCHASE						= 73029,		// 兑换时因为限购，不能兑换
		ERR_LEGION_BUILDING_CREATE_LIMIT					= 73030,		// 创建军团建筑时，已到可建造数量上限
		ERR_LEGION_BUILDING_CANNOT_FIND						= 73031,		// 升级军团建筑时，没找到该建筑
		ERR_LEGION_BUILDING_UPGRADING					    = 73032,		// 升级军团建筑时，该建筑正在升级中
		ERR_LEGION_BUILDING_UPGRADE_MAX					    = 73033,		// 升级军团建筑时，已经是最高等级，不能继续升级
		ERR_LEGION_BUILDING_UPGRADE_LACK					= 73034,		// 升级军团建筑时，所需升级消耗资源不足
		ERR_LEGION_BUILDING_UPGRADE_LIMIT					= 73035,		// 升级军团建筑时，所需升级条件不满足
		ERR_LEGION_BUILDING_NOTIN_UPGRADING					= 73036,		// 取消升级军团建筑时，发现目前没有在升级
		ERR_LEGION_OPEN_GRUBE_INCDTIMR						= 73037,		// 开启货仓时，发现没有过CD时间
		ERR_LEGION_OPEN_GRUBE_RESIDUE						= 73038,		// 开启货仓时，发现资源有剩余
		ERR_LEGION_OPEN_GRUBE_LACK							= 73039,		// 开启货仓时，条件不足
		ERR_LEGION_REPAIR_GRUBE_HPERROR						= 73040,		// 维修货仓时，血量错误
		ERR_LEGION_UPGRADE_NEED_LACK						= 73041,		// 升级军团时，升级所需消耗资源不足
		ERR_LEGION_UPGRADE_NEEDRESOURCE_ERROR				= 73042,		// 升级军团时，升级所需升级条件不满足
		ERR_LEGION_OPEN_GRUBE_DESTRUCT						= 73043,		// 开启货仓时，因为处于击毁状态不能开启
		ERR_LEGION_UPGRADE_GRUBE_LEFT						= 73044,		// 升级货仓时，货仓中有剩余资源，不可升级
		ERR_LEGION_GATHER_IN_LEAVE_TIME						= 73045,		// 处于离开等待时，不能采集
	};
}

}

#endif
