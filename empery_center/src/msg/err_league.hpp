#ifndef EMPERY_CENTER_MSG_ERR_LEAGUE_HPP_
#define EMPERY_CENTER_MSG_ERR_LEAGUE_HPP_

#include <poseidon/cbpp/status_codes.hpp>

namespace EmperyCenter {

namespace Msg {
	using namespace Poseidon::Cbpp::StatusCodes;

	enum {
		ERR_ACCOUNT_HAVE_LEAGUE                 			= 74000,	  	// 已经有联盟
		ERR_LEAGUE_NOT_JOIN_LEGION              			= 74001,		// 没有加入军团
		ERR_LEAGUE_NO_POWER_LEGION							= 74002,		// 没有权限
		ERR_LEAGUE_CREATE_NOTENOUGH_MONEY     				= 74003,        // 创建资源不足
		ERR_LEAGUE_CREATE_HOMONYM              				= 74004,		// 联盟名已存在
		ERR_LEAGUE_CANNOT_FIND             					= 74005,		// 没有找到联盟
		ERR_LEAGUE_SEARCH_BY_LEADNAME_CANNOT_FIND           = 74006,		// 查找联盟时，没有根据盟主昵称找到军团信息
		ERR_LEAGUE_ALREADY_APPLY							= 74007,		// 已经申请过同联盟
		ERR_LEAGUE_CANNOT_FIND_DATA							= 74008,		// 数据不存在
		ERR_LEAGUE_APPLY_COUNT_LIMIT						= 74009,		// 当前可申请联盟数量已达上限
		ERR_LEAGUE_MEMBER_FULL								= 74010,		// 联盟人数已满
		ERR_LEAGUE_NO_POWER									= 74011,		// 没有相应权限
		ERR_LEAGUE_NOT_JOIN									= 74012,		// 没有加入联盟
		ERR_LEGION_CANNOT_FIND             					= 74013,		// 没有找到军团
		ERR_LEAGUE_APPLYJOIN_CANNOT_FIND_DATA				= 74014,		// 没有找到请求加入联盟的申请数据
		ERR_LEAGUE_ALREADY_INVATE							= 74015,		// 已经邀请过
		ERR_LEAGUE_NOTFIND_INVITE_INFO						= 74016,		// 没有找到邀请信息
		ERR_LEAGUE_QUIT_IN_WAITTIME						    = 74017,		// 已经在退出等待中了
		ERR_LEAGUE_KICK_IN_WAITTIME							= 74018,		// 已经在被移除等待中了
		ERR_LEAGUE_NOT_IN_SAME_LEAGUE						= 74019,		// 不属于同一联盟
		ERR_LEAGUE_ERROR_LEAGUELEADER           			= 74020,		// 盟主需要先转让给别人
		/*
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
		*/
	};
}

}

#endif
