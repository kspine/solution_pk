#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../utilities.hpp"
#include "../mysql/wd_slip.hpp"
#include "../bill_states.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("withdraw", session, params){
	const auto &login_name = params.at("loginName");
	const auto delta_balance = boost::lexical_cast<std::uint64_t>(params.at("deltaBalance"));
	const auto &deal_password = params.at("dealPassword");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if(AccountMap::get_password_hash(deal_password) != info.deal_password_hash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	if((info.banned_until != 0) && (Poseidon::get_utc_time() < info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	const auto wd_slip_serial_prefix = get_config<std::string>("wd_slip_serial_prefix", "PKWD");
	const double withdrawal_fee_ratio = get_config<double>("withdrawal_fee_ratio", 0.05);

	const auto serial = generate_bill_serial(wd_slip_serial_prefix);
	const auto utc_now = Poseidon::get_utc_time();

	auto amount = delta_balance;
	auto fee = static_cast<std::uint64_t>(std::ceil(delta_balance * withdrawal_fee_ratio - 0.001));
	if(fee > amount){
		fee = amount;
	}
	amount -= fee;

	const auto wd_slip_obj = boost::make_shared<MySql::Promotion_WdSlip>(
		serial, utc_now, amount, fee, info.account_id.get(), BillStates::ST_NEW, std::string(), remarks);

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(info.account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, delta_balance,
		Events::ItemChanged::R_WITHDRAW, info.account_id.get(), 0, 0, remarks);
	transaction.emplace_back(info.account_id,ItemTransactionElement::OP_ADD, ItemIds::ID_WITHDRAWN_BALANCE, amount,
		Events::ItemChanged::R_WITHDRAW, info.account_id.get(), 0, 0, remarks);
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}

	LOG_EMPERY_PROMOTION_INFO("Created withdraw slip: serial = ", serial,
		", amount = ", amount, ", fee = ", fee, ", account_id = ", info.account_id, ", remarks = ", remarks);
	wd_slip_obj->async_save(true);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
