#include "precompiled.hpp"
#include "payment_transaction.hpp"
#include "singletons/payment_transaction_map.hpp"
#include "mysql/payment_transaction.hpp"
#include "singletons/item_box_map.hpp"
#include "item_box.hpp"
#include "transaction_element.hpp"
#include "item_ids.hpp"
#include "reason_ids.hpp"

namespace EmperyCenter {

PaymentTransaction::PaymentTransaction(std::string serial, AccountUuid account_uuid,
	std::uint64_t created_time, std::uint64_t expiry_time, std::uint64_t amount, std::string remarks)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_PaymentTransaction>(std::move(serial), account_uuid.get(),
				created_time, expiry_time, amount, std::move(remarks), false, false, std::string());
			obj->save_and_wait(false);
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
std::uint64_t PaymentTransaction::get_amount() const {
	return m_obj->get_amount();
}

bool PaymentTransaction::has_been_settled() const {
	return m_obj->get_settled();
}
bool PaymentTransaction::has_been_cancelled() const {
	return m_obj->get_cancelled();
}
const std::string &PaymentTransaction::get_operation_remarks() const {
	return m_obj->unlocked_get_operation_remarks();
}
void PaymentTransaction::settle(std::string operation_remarks){
	PROFILE_ME;

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_ERROR("Payment transaction has been virtually removed: serial = ", get_serial(),
			", expiry_time = ", get_expiry_time(), ", settled = ", has_been_settled(), ", cancelled = ", has_been_cancelled());
		DEBUG_THROW(Exception, sslit("Payment transaction has been virtually removed"));
	}

	const auto item_box = ItemBoxMap::require(get_account_uuid());

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_ADD, ItemIds::ID_DIAMONDS, get_amount(),
		ReasonIds::ID_PAYMENT, 0, 0, 0);
	item_box->commit_transaction(transaction,
		[&]{
			m_obj->set_settled(true);
			m_obj->set_operation_remarks(std::move(operation_remarks));
		});

	PaymentTransactionMap::update(virtual_shared_from_this<PaymentTransaction>(), false);
}
void PaymentTransaction::cancel(std::string operation_remarks){
	PROFILE_ME;

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_ERROR("Payment transaction has been virtually removed: serial = ", get_serial(),
			", expiry_time = ", get_expiry_time(), ", settled = ", has_been_settled(), ", cancelled = ", has_been_cancelled());
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

	if(has_been_settled()){
		LOG_EMPERY_CENTER_DEBUG("Payment transaction settled: serial = ", get_serial());
		return true;
	}
	if(has_been_cancelled()){
		LOG_EMPERY_CENTER_DEBUG("Payment transaction cancelled: serial = ", get_serial());
		return true;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(get_expiry_time() <= utc_now){
		LOG_EMPERY_CENTER_DEBUG("Payment transaction expired: serial = ", get_serial());
		return true;
	}
	return false;
}

}
