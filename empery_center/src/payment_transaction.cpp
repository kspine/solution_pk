#include "precompiled.hpp"
#include "payment_transaction.hpp"
#include "singletons/payment_transaction_map.hpp"
#include "mysql/payment.hpp"
#include "item_box.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "events/account.hpp"
#include "mail_box.hpp"
#include "mail_data.hpp"
#include "mail_type_ids.hpp"
#include "chat_message_slot_ids.hpp"
#include "data/global.hpp"
#include "item_ids.hpp"
#include "account_utilities.hpp"
#include "data/item.hpp"
#include "singletons/mail_box_map.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/csv_parser.hpp>

namespace EmperyCenter {

namespace {
	std::uint64_t g_auto_inc = Poseidon::get_utc_time();

	boost::container::flat_map<unsigned, std::uint64_t> g_level_prices;

	MODULE_RAII_PRIORITY(/* handles */, 1000){
		Poseidon::CsvParser csv;

		boost::container::flat_map<unsigned, std::uint64_t> level_prices;
		level_prices.reserve(20);
		constexpr char path[] = "empery_promotion_levels.csv";
		LOG_EMPERY_CENTER_INFO("Loading csv file: path = ", path);
		csv.load(path);
		while(csv.fetch_row()){
			unsigned level = 0;
			std::uint64_t price = 0;

			csv.get(level, "displayLevel");
			csv.get(price, "immediatePrice");

			if(!level_prices.emplace(level, price).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate promotion level: level = ", level);
				DEBUG_THROW(Exception, sslit("Duplicate promotion level"));
			}
		}
		g_level_prices = std::move(level_prices);
	}
}

std::string PaymentTransaction::random_serial(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	const auto dt = Poseidon::break_down_time(utc_now);

	char str[64];
	unsigned len = (unsigned)std::sprintf(str, "%04u%02u%02u%02u%02u%02u%06u",
		dt.yr, dt.mon, dt.day, dt.hr, dt.min, dt.sec, (unsigned)(++g_auto_inc % 1000000));
	return std::string(str, len);
}

PaymentTransaction::PaymentTransaction(std::string serial, AccountUuid account_uuid, std::uint64_t created_time, std::uint64_t expiry_time,
	ItemId item_id, std::uint64_t item_count, std::string remarks)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_PaymentTransaction>(std::move(serial), account_uuid.get(), created_time, expiry_time,
				item_id.get(), item_count, std::move(remarks), false, false, std::string());
			obj->async_save(true);
			return obj;
		}())
{
}
PaymentTransaction::PaymentTransaction(boost::shared_ptr<MySql::Center_PaymentTransaction> obj)
	: m_obj(std::move(obj))
{
}
PaymentTransaction::~PaymentTransaction(){
}

const std::string &PaymentTransaction::get_serial() const {
	return m_obj->unlocked_get_serial();
}
AccountUuid PaymentTransaction::get_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_account_uuid());
}
std::uint64_t PaymentTransaction::get_created_time() const {
	return m_obj->get_created_time();
}
std::uint64_t PaymentTransaction::get_expiry_time() const {
	return m_obj->get_expiry_time();
}

ItemId PaymentTransaction::get_item_id() const {
	return ItemId(m_obj->get_item_id());
}
std::uint64_t PaymentTransaction::get_item_count() const {
	return m_obj->get_item_count();
}

bool PaymentTransaction::has_been_committed() const {
	return m_obj->get_committed();
}
bool PaymentTransaction::has_been_cancelled() const {
	return m_obj->get_cancelled();
}
const std::string &PaymentTransaction::get_operation_remarks() const {
	return m_obj->unlocked_get_operation_remarks();
}
void PaymentTransaction::commit(const boost::shared_ptr<ItemBox> &item_box, const boost::shared_ptr<MailBox> &mail_box,
	std::string operation_remarks)
{
	PROFILE_ME;

	const auto account_uuid = get_account_uuid();

	const auto account = AccountMap::require(account_uuid);

	Poseidon::sync_raise_event(
		boost::make_shared<Events::AccountSynchronizeWithThirdServer>(
			boost::shared_ptr<long>(), account->get_platform_id(), account->get_login_name(), std::string()));

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_ERROR("Payment transaction has been virtually removed: serial = ", get_serial(),
			", expiry_time = ", get_expiry_time(), ", committed = ", has_been_committed(), ", cancelled = ", has_been_cancelled());
		DEBUG_THROW(Exception, sslit("Payment transaction has been virtually removed"));
	}

	const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
	const auto language_id = LanguageId(); // neutral

	const auto default_mail_expiry_duration = Data::Global::as_unsigned(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION);
	const auto expiry_duration = checked_mul(default_mail_expiry_duration, (std::uint64_t)60000);
	const auto utc_now = Poseidon::get_utc_time();

	const auto item_id = get_item_id();
	const auto item_count = get_item_count();

	const auto item_data = Data::Item::require(item_id);

	std::uint64_t taxing_amount = 0;
	if((item_data->type.first == Data::Item::CAT_LEVEL_GIFT_BOX) || (item_data->type.first == Data::Item::CAT_LARGE_LEVEL_GIFT_BOX)){
		const auto level = item_data->type.second;
		LOG_EMPERY_CENTER_DEBUG("Gift box: item_id = ", item_id, ", type = ", (unsigned)item_data->type.first,
			", level = ", level, ", item_count = ", item_count);
		const auto unit_price = g_level_prices.at(level);
		taxing_amount = checked_mul(unit_price, item_count);

		account->activate();
	}

	MailTypeId mail_type_id;
	if(item_id == ItemIds::ID_DIAMONDS){
		mail_type_id = MailTypeIds::ID_PAYMENT_DIAMONDS;
	} else if(item_id == ItemIds::ID_GOLD_COINS){
		mail_type_id = MailTypeIds::ID_PAYMENT_GOLD_COINS;
	} else {
		mail_type_id = MailTypeIds::ID_PAYMENT_GIFT_BOXES;
	}

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	segments.reserve(2);
	segments.emplace_back(ChatMessageSlotIds::ID_ITEM_ID,    boost::lexical_cast<std::string>(item_id));
	segments.emplace_back(ChatMessageSlotIds::ID_ITEM_COUNT, boost::lexical_cast<std::string>(item_count));

	boost::container::flat_map<ItemId, std::uint64_t> attachments;
	// attachments.emplace(get_item_id(), item_count_unlocked);

	const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
		mail_type_id, AccountUuid(), std::string(), std::move(segments), std::move(attachments));
	MailBoxMap::insert_mail_data(mail_data);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_ADD, item_id, item_count,
		ReasonIds::ID_PAYMENT, 0, 0, 0);
	item_box->commit_transaction(transaction, false,
		[&]{
			MailBox::MailInfo mail_info = { };
			mail_info.mail_uuid   = mail_uuid;
			mail_info.expiry_time = saturated_add(utc_now, expiry_duration);
			mail_info.system      = true;
            mail_box->insert(std::move(mail_info));

			m_obj->set_committed(true);
			m_obj->set_operation_remarks(std::move(operation_remarks));
		});

	try {
		accumulate_promotion_bonus(account_uuid, taxing_amount);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
	}

	PaymentTransactionMap::update(virtual_shared_from_this<PaymentTransaction>(), false);
}
void PaymentTransaction::cancel(std::string operation_remarks){
	PROFILE_ME;

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_ERROR("Payment transaction has been virtually removed: serial = ", get_serial(),
			", expiry_time = ", get_expiry_time(), ", committed = ", has_been_committed(), ", cancelled = ", has_been_cancelled());
		DEBUG_THROW(Exception, sslit("Payment transaction has been virtually removed"));
	}

	m_obj->set_cancelled(true);
	m_obj->set_operation_remarks(std::move(operation_remarks));

	PaymentTransactionMap::update(virtual_shared_from_this<PaymentTransaction>(), false);
}

const std::string &PaymentTransaction::get_remarks() const {
	return m_obj->unlocked_get_remarks();
}
void PaymentTransaction::set_remarks(std::string remarks){
	m_obj->set_remarks(std::move(remarks));
}

bool PaymentTransaction::is_virtually_removed() const {
	PROFILE_ME;

	if(has_been_committed()){
		return true;
	}
	if(has_been_cancelled()){
		return true;
	}
	return false;
}

}
