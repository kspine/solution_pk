#include "precompiled.hpp"
#include "account.hpp"
#include "mmain.hpp"
#include "mysql/legion.hpp"
#include "singletons/legion_map.hpp"
#include "singletons/legion_building_map.hpp"
#include "msg/cs_legion.hpp"
#include "player_session.hpp"
#include "legion.hpp"
#include "legion_attribute_ids.hpp"
#include "singletons/account_map.hpp"
#include "singletons/legion_applyjoin_map.hpp"
#include "singletons/legion_invitejoin_map.hpp"
#include "chat_message_type_ids.hpp"
#include "data/global.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_box.hpp"
#include "mail_data.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "singletons/legion_task_box_map.hpp"

namespace EmperyCenter {

std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<Legion>> Legion::async_create(
	LegionUuid legion_uuid,  std::string legion_name, AccountUuid account_uuid, std::uint64_t created_time)
{
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::Center_Legion>(legion_uuid.get(), std::move(legion_name),account_uuid.get(),created_time);
	obj->enable_auto_saving();
	auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);
	auto account = boost::make_shared<Legion>(std::move(obj), std::vector<boost::shared_ptr<MySql::Center_LegionAttribute>>());
	return std::make_pair(std::move(promise), std::move(account));
}

Legion::Legion(boost::shared_ptr<MySql::Center_Legion> obj,
	const std::vector<boost::shared_ptr<MySql::Center_LegionAttribute>> &attributes)
	: m_obj(std::move(obj))
{
	for(auto it = attributes.begin(); it != attributes.end(); ++it){
		m_attributes.emplace(LegionAttributeId((*it)->get_legion_attribute_id()), *it);
	}

//	LOG_EMPERY_CENTER_INFO("legion members size==============================================",m_members.size());
//	sleep(5000);
}
Legion::~Legion(){
}

LegionUuid Legion::get_legion_uuid() const {
	return LegionUuid(m_obj->unlocked_get_legion_uuid());
}

AccountUuid Legion::get_account_uuid() const{
	return AccountUuid(m_obj->unlocked_get_creater_uuid());
}

void Legion::InitAttributes(AccountUuid accountid,std::string content, std::string language, std::string icon,unsigned bshenhe)
{
	PROFILE_ME;

	// 设置属性
	boost::container::flat_map<LegionAttributeId, std::string> modifiers;
	modifiers.emplace(LegionAttributeIds::ID_LEADER, accountid.str());
	modifiers.emplace(LegionAttributeIds::ID_LEVEL, "1");
	modifiers.emplace(LegionAttributeIds::ID_CONTENT, std::move(content));
	modifiers.emplace(LegionAttributeIds::ID_ICON, std::move(icon));
	modifiers.emplace(LegionAttributeIds::ID_LANAGE, std::move(language));
	if(bshenhe)
	{
		modifiers.emplace(LegionAttributeIds::ID_AUTOJOIN, "1");
	}
	else
	{
		modifiers.emplace(LegionAttributeIds::ID_AUTOJOIN,"0");
	}
	modifiers.emplace(LegionAttributeIds::ID_MONEY, "0");
	modifiers.emplace(LegionAttributeIds::ID_RANK, "0");
	set_attributes(std::move(modifiers));
}


//void Legion::AddMember(AccountUuid accountid,unsigned rank,std::uint64_t join_time,std::string  donate,std::string  weekdonate,std::string nick)
void Legion::AddMember(boost::shared_ptr<Account> account,unsigned rank,std::uint64_t join_time)
{
	// 先判断玩家是否已经加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account->get_account_uuid());
	if(member)
	{
		LOG_EMPERY_CENTER_ERROR("AddMember Find same  account_uuid:", account->get_account_uuid());
	}
	else
	{
		auto pair = LegionMember::async_create( get_legion_uuid(),account->get_account_uuid(),join_time);
		Poseidon::JobDispatcher::yield(pair.first, true);

		auto member = std::move(pair.second);

		// 设置成员属性
		member->InitAttributes(account,rank);

	//	m_members.emplace(AccountUuid((*it)->get_account_uuid()), *it);

	//	m_members.emplace(accountid,member);

		LOG_EMPERY_CENTER_DEBUG("AddMember account_uuid:", account->get_account_uuid(),"  join_time:",join_time," rank:",rank);

		LegionMemberMap::insert(member, std::string());

		Msg::SC_LegionNoticeMsg msg;
		msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_JOIN;
		msg.nick = account->get_nick();
		msg.ext1 = "";
		sendNoticeMsg(msg);
	}
}

/*
const std::string &Legion::get_login_name() const {
	return m_obj->unlocked_get_login_name();
}
*/
LegionUuid Legion::get_creater_uuid() const {
	return LegionUuid(std::move(m_obj->unlocked_get_creater_uuid()));
}
/*
unsigned Legion::get_promotion_level() const {
	return m_obj->get_promotion_level();
}

void Legion::set_promotion_level(unsigned promotion_level){
	PROFILE_ME;

	m_obj->set_promotion_level(promotion_level);

	LegionMap::update(virtual_shared_from_this<Legion>(), false);
}
*/
const std::string &Legion::get_nick() const {
	return m_obj->unlocked_get_name();
}
void Legion::set_nick(std::string nick){
	PROFILE_ME;

	m_obj->set_name(std::move(nick));

	LegionMap::update(virtual_shared_from_this<Legion>(), false);
}

std::uint64_t Legion::get_created_time() const {
	return m_obj->get_created_time();
}

/*
bool Legion::has_been_activated() const {
	return m_obj->get_activated();
}
void Legion::set_activated(bool activated){
	PROFILE_ME;

	m_obj->set_activated(activated);

	LegionMap::update(virtual_shared_from_this<Legion>(), false);
}

std::uint64_t Legion::get_banned_until() const {
	return m_obj->get_banned_until();
}
void Legion::set_banned_until(std::uint64_t banned_until){
	m_obj->set_banned_until(banned_until);

	LegionMap::update(virtual_shared_from_this<Legion>(), false);
}

std::uint64_t Legion::get_quieted_until() const {
	return m_obj->get_quieted_until();
}
void Legion::set_quieted_until(std::uint64_t quieted_until){
	m_obj->set_quieted_until(quieted_until);

	LegionMap::update(virtual_shared_from_this<Legion>(), false);
}
*/
const std::string &Legion::get_attribute(LegionAttributeId account_attribute_id) const {
	PROFILE_ME;

	const auto it = m_attributes.find(account_attribute_id);
	if(it == m_attributes.end()){
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void Legion::get_attributes(boost::container::flat_map<LegionAttributeId, std::string> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		ret[it->first] = it->second->unlocked_get_value();
	}
}
void Legion::set_attributes(boost::container::flat_map<LegionAttributeId, std::string> modifiers){
	PROFILE_ME;


	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto obj_it = m_attributes.find(it->first);
		if(obj_it == m_attributes.end()){
			auto obj = boost::make_shared<MySql::Center_LegionAttribute>(m_obj->get_legion_uuid(),
				it->first.get(), std::string());
			obj->async_save(true);
			m_attributes.emplace(it->first, std::move(obj));
		}
	}

	for(auto it = modifiers.begin(); it != modifiers.end(); ++it){
		const auto &obj = m_attributes.at(it->first);
		obj->set_value(std::move(it->second));

		LOG_EMPERY_CENTER_DEBUG("Legion::set_attributes  account_uuid= ", obj->get_legion_uuid(), " key:",it->first,"  value:",std::move(it->second));
	}

	LegionMap::update(virtual_shared_from_this<Legion>(), false);
}

void Legion::synchronize_with_player(AccountUuid account_uuid,const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	Msg::SC_LegionInfo msg;
	msg.legion_uuid     		= 	get_legion_uuid().str();
	msg.legion_name     		= 	get_nick();
	// 要根据军团长id 获得军团长信息
	const auto leader_account = AccountMap::require(AccountUuid(get_attribute(LegionAttributeIds::ID_LEADER)));
	msg.legion_leadername    	= 	leader_account->get_nick();
	msg.legion_icon     		= 	get_attribute(LegionAttributeIds::ID_ICON);
	msg.legion_notice     		= 	get_attribute(LegionAttributeIds::ID_CONTENT);
	msg.legion_level     		= 	get_attribute(LegionAttributeIds::ID_LEVEL);
	msg.legion_rank     		= 	get_attribute(LegionAttributeIds::ID_RANK);
	msg.legion_money     		= 	get_attribute(LegionAttributeIds::ID_MONEY);

	// 获得军团成员个人信息
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 个人信息
		msg.legion_titleid     		= 	member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
		msg.legion_donate     		= 	member->get_attribute(LegionMemberAttributeIds::ID_DONATE);
	}

	// 军团成员数量
	msg.legion_member_count     = 	boost::lexical_cast<std::string>(LegionMemberMap::get_legion_member_count(get_legion_uuid()));

	LOG_EMPERY_CENTER_INFO("legion members size==============================================",msg.legion_member_count);

	session->send(msg);

	/*
	Msg::SC_LegionInfo msg;
	msg.account_uuid    = get_account_uuid().str();
	msg.nick            = get_nick();
	msg.attributes.reserve(m_attributes.size());
	for(auto it = m_attributes.begin(); it != m_attributes.end(); ++it){
		auto &attribute = *msg.attributes.emplace(msg.attributes.end());
		attribute.account_attribute_id = it->first.get();
		attribute.value                = it->second->unlocked_get_value();
	}
	msg.promotion_level = get_promotion_level();
	msg.activated       = has_been_activated();
	session->send(msg);
	*/
}

void Legion::disband()
{
	// 清除军团属性
	for(auto it = m_attributes.begin(); it != m_attributes.end(); )
	{
		it =  m_attributes.erase(it);
	}

	// 从数据库中删除军团属性
	std::string strsql = "DELETE FROM Center_LegionAttribute WHERE legion_uuid='";
	strsql += get_legion_uuid().str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_deleting("Center_LegionAttribute",strsql);

	// 设置对应的联盟信息为空
	set_member_league_uuid("");

	// 军团解散的成员的善后操作
	LegionMemberMap::disband_legion(get_legion_uuid());


	// 清空请求加入的数据
	LegionApplyJoinMap::deleteInfo_by_legion_uuid(get_legion_uuid());

	// 清空邀请加入的数据
	LegionInviteJoinMap::deleteInfo_by_legion_uuid(get_legion_uuid());

	// 同时清空相关的军团建筑
	LegionBuildingMap::deleteInfo_by_legion_uuid(get_legion_uuid());

	// 卸载军团任务
	LegionTaskBoxMap::unload(get_legion_uuid());
}

void Legion::sendNoticeMsg(Msg::SC_LegionNoticeMsg msg)
{
	std::vector<boost::shared_ptr<LegionMember>> members;
	LegionMemberMap::get_by_legion_uuid(members, get_legion_uuid());

	for(auto it = members.begin(); it != members.end(); ++it)
	{
		// 判断玩家是否在线
		const auto target_session = PlayerSessionMap::get((*it)->get_account_uuid());
		if(target_session)
		{
			target_session->send(msg);
		}
	}
}

void Legion::sendmail(boost::shared_ptr<Account> account, ChatMessageTypeId ntype,std::string str)
{
	const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
//	const auto language_id = LanguageId(legion->get_attribute(LegionAttributeIds::ID_LANAGE));

	const auto language_id = LanguageId(boost::lexical_cast<unsigned int>(get_attribute(LegionAttributeIds::ID_LANAGE)));

	const auto default_mail_expiry_duration = Data::Global::as_double(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION);
	const auto expiry_duration = static_cast<std::uint64_t>(default_mail_expiry_duration * 60000);
	const auto utc_now = Poseidon::get_utc_time();

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	boost::container::flat_map<ItemId, std::uint64_t> attachments;

//	std::string str = LegionUuid(get_legion_uuid()).str() + "," + get_nick();
	const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
		ntype, AccountUuid(), std::move(str), std::move(segments), std::move(attachments));
	MailBoxMap::insert_mail_data(mail_data);

	AccountMap::require_controller_token(account->get_account_uuid(),"");

	const auto to_mail_box = MailBoxMap::require(account->get_account_uuid());
	to_mail_box->pump_status();

	MailBox::MailInfo mail_info = { };
	mail_info.system = true;
	mail_info.mail_uuid   = mail_uuid;
	mail_info.expiry_time = saturated_add(utc_now, expiry_duration);
	to_mail_box->insert(std::move(mail_info));
}

void Legion::broadcast_to_members(std::uint16_t message_id, const Poseidon::StreamBuffer &payload){
	PROFILE_ME;
	std::vector<boost::shared_ptr<LegionMember>> members;
	LegionMemberMap::get_by_legion_uuid(members, get_legion_uuid());

	for(auto it = members.begin(); it != members.end(); ++it)
	{
		// 判断玩家是否在线
		const auto session = PlayerSessionMap::get((*it)->get_account_uuid());
		if(session)
		{
			try {
				session->send(message_id, payload);
				LOG_EMPERY_CENTER_DEBUG("broadcast_to_legion members message_id:",message_id," msg_data = ", payload.dump());
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}

void Legion::set_member_league_uuid(std::string str_league_uuid)
{
	std::vector<boost::shared_ptr<LegionMember>> members;
	LegionMemberMap::get_by_legion_uuid(members, get_legion_uuid());

	for(auto it = members.begin(); it != members.end(); ++it)
	{
		auto member = *it;

		member->set_league_uuid(str_league_uuid);
	}
}

}