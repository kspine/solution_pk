#include "precompiled.hpp"
#include "auction_transaction.hpp"
#include "singletons/auction_transaction_map.hpp"
#include "mysql/auction.hpp"
#include "singletons/auction_center_map.hpp"
#include "auction_center.hpp"
#include "transaction_element.hpp"
#include "reason_ids.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_box.hpp"
#include "mail_data.hpp"
#include "mail_type_ids.hpp"
#include "chat_message_slot_ids.hpp"
#include "data/global.hpp"
#include "data/item.hpp"

namespace EmperyCenter {

namespace {
	std::uint64_t g_auto_inc = Poseidon::get_utc_time();
}

std::string AuctionTransaction::random_serial(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();
	const auto dt = Poseidon::break_down_time(utc_now);

	char str[64];
	unsigned len = (unsigned)std::sprintf(str, "AUCT_%04u%02u%02u%02u%02u%02u%06u",
		dt.yr, dt.mon, dt.day, dt.hr, dt.min, dt.sec, (unsigned)(++g_auto_inc % 1000000));
	return std::string(str, len);
}

AuctionTransaction::AuctionTransaction(std::string serial, AccountUuid account_uuid, AuctionTransaction::Operation operation,
	std::uint64_t created_time, std::uint64_t expiry_time, ItemId item_id, std::uint64_t item_count, std::string remarks)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_AuctionTransaction>(std::move(serial), account_uuid.get(), static_cast<unsigned>(operation),
				created_time, expiry_time, item_id.get(), item_count, std::move(remarks), false, false, std::string());
			obj->async_save(true);
			return obj;
		}())
{
}
AuctionTransaction::AuctionTransaction(boost::shared_ptr<MySql::Center_AuctionTransaction> obj)
	: m_obj(std::move(obj))
{
}
AuctionTransaction::~AuctionTransaction(){
}

const std::string &AuctionTransaction::get_serial() const {
	return m_obj->unlocked_get_serial();
}
AccountUuid AuctionTransaction::get_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_account_uuid());
}
AuctionTransaction::Operation AuctionTransaction::get_operation() const {
	return static_cast<Operation>(m_obj->get_operation());
}
std::uint64_t AuctionTransaction::get_created_time() const {
	return m_obj->get_created_time();
}
std::uint64_t AuctionTransaction::get_expiry_time() const {
	return m_obj->get_expiry_time();
}

ItemId AuctionTransaction::get_item_id() const {
	return ItemId(m_obj->get_item_id());
}
std::uint64_t AuctionTransaction::get_item_count() const {
	return m_obj->get_item_count();
}

bool AuctionTransaction::has_been_committed() const {
	return m_obj->get_committed();
}
bool AuctionTransaction::has_been_cancelled() const {
	return m_obj->get_cancelled();
}
const std::string &AuctionTransaction::get_operation_remarks() const {
	return m_obj->unlocked_get_operation_remarks();
}
void AuctionTransaction::commit(std::string operation_remarks){
	PROFILE_ME;

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_ERROR("Auction transaction has been virtually removed: serial = ", get_serial(),
			", expiry_time = ", get_expiry_time(), ", committed = ", has_been_committed(), ", cancelled = ", has_been_cancelled());
		DEBUG_THROW(Exception, sslit("Auction transaction has been virtually removed"));
	}

	const auto operation = get_operation();
	switch(operation){
	case OP_TRANSFER_IN:
		{
			const auto mail_box = MailBoxMap::require(get_account_uuid());

			const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
			const auto language_id = LanguageId(); // neutral

			const auto default_mail_expiry_duration = Data::Global::as_unsigned(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION);
			const auto expiry_duration = checked_mul(default_mail_expiry_duration, (std::uint64_t)60000);
			const auto utc_now = Poseidon::get_utc_time();

			const auto resource_amount_per_box = Data::Global::as_unsigned(Data::Global::SLOT_AUCTION_TRANSFER_RESOURCE_AMOUNT_PER_BOX);
			const auto item_count_per_box = Data::Global::as_unsigned(Data::Global::SLOT_AUCTION_TRANSFER_ITEM_COUNT_PER_BOX);

			const auto item_id = get_item_id();
			std::uint64_t item_count_unlocked = 0;
			const auto item_data = Data::Item::require(item_id);
			const auto item_box_count = get_item_count();
			if(item_data->type.first == Data::Item::CAT_RESOURCE_BOX){
				item_count_unlocked = checked_mul(item_box_count, resource_amount_per_box) / item_data->value;
			} else {
				item_count_unlocked = checked_mul(item_box_count, item_count_per_box);
			}

			std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
			segments.reserve(2);
			segments.emplace_back(ChatMessageSlotIds::ID_ITEM_ID,    boost::lexical_cast<std::string>(item_id));
			segments.emplace_back(ChatMessageSlotIds::ID_ITEM_COUNT, boost::lexical_cast<std::string>(item_box_count));

			boost::container::flat_map<ItemId, std::uint64_t> attachments;
			attachments.emplace(item_id, item_count_unlocked);

			const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
				MailTypeIds::ID_AUCTION_RESULT, AccountUuid(), std::string(), std::move(segments), std::move(attachments));
			MailBoxMap::insert_mail_data(mail_data);

			MailBox::MailInfo mail_info = { };
			mail_info.mail_uuid   = mail_uuid;
			mail_info.expiry_time = saturated_add(utc_now, expiry_duration);
			mail_info.system      = true;
			mail_box->insert(std::move(mail_info));

			m_obj->set_committed(true);
			m_obj->set_operation_remarks(std::move(operation_remarks));
		}
		break;

	case OP_TRANSFER_OUT:
		{
			const auto auction_center = AuctionCenterMap::require(get_account_uuid());

			std::vector<AuctionTransactionElement> transaction;
			transaction.emplace_back(AuctionTransactionElement::OP_REMOVE, get_item_id(), get_item_count(),
				ReasonIds::ID_AUCTION_CENTER_WITHDRAW, 0, 0, 0);
			auction_center->commit_item_transaction(transaction,
				[&]{
					m_obj->set_committed(true);
					m_obj->set_operation_remarks(std::move(operation_remarks));
				});
		}
		break;

	default:
		LOG_EMPERY_CENTER_ERROR("Unknown auction transaction operation: ", (unsigned)operation);
		DEBUG_THROW(Exception, sslit("Unknown auction transaction operation"));
	}

	AuctionTransactionMap::update(virtual_shared_from_this<AuctionTransaction>(), false);
}
void AuctionTransaction::cancel(std::string operation_remarks){
	PROFILE_ME;

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_ERROR("Auction transaction has been virtually removed: serial = ", get_serial(),
			", expiry_time = ", get_expiry_time(), ", committed = ", has_been_committed(), ", cancelled = ", has_been_cancelled());
		DEBUG_THROW(Exception, sslit("Auction transaction has been virtually removed"));
	}

	m_obj->set_cancelled(true);
	m_obj->set_operation_remarks(std::move(operation_remarks));

	AuctionTransactionMap::update(virtual_shared_from_this<AuctionTransaction>(), false);
}

const std::string &AuctionTransaction::get_remarks() const {
	return m_obj->unlocked_get_remarks();
}
void AuctionTransaction::set_remarks(std::string remarks){
	m_obj->set_remarks(std::move(remarks));
}

bool AuctionTransaction::is_virtually_removed() const {
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
