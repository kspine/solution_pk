#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/account_map.hpp"
#include "../msg/err_account.hpp"
#include "../singletons/item_map.hpp"
#include "../item_transaction_element.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"

namespace EmperyPromotion {

ACCOUNT_SERVLET("transfer", session, params){
	const auto &login_name = params.at("loginName");
	const auto delta_balance = boost::lexical_cast<boost::uint64_t>(params.at("deltaBalance"));
	const auto &deal_password = params.at("dealPassword");
	const auto &to_login_name = params.at("toLoginName");
	const auto &remarks = params.get("remarks");

	Poseidon::JsonObject ret;
	auto src_info = AccountMap::get_by_login_name(login_name);
	if(Poseidon::has_none_flags_of(src_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_SUCH_ACCOUNT;
		ret[sslit("errorMessage")] = "Account is not found";
		return ret;
	}
	if(AccountMap::get_password_hash(deal_password) != src_info.deal_password_hash){
		ret[sslit("errorCode")] = (int)Msg::ERR_INVALID_DEAL_PASSWORD;
		ret[sslit("errorMessage")] = "Deal password is incorrect";
		return ret;
	}
	if((src_info.banned_until != 0) && (Poseidon::get_utc_time() < src_info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_ACCOUNT_BANNED;
		ret[sslit("errorMessage")] = "Account is banned";
		return ret;
	}

	auto dst_info = AccountMap::get_by_login_name(to_login_name);
	if(Poseidon::has_none_flags_of(dst_info.flags, AccountMap::FL_VALID)){
		ret[sslit("errorCode")] = (int)Msg::ERR_TRANSFER_DEST_NOT_FOUND;
		ret[sslit("errorMessage")] = "Destination account is not found";
		return ret;
	}
	if((dst_info.banned_until != 0) && (Poseidon::get_utc_time() < dst_info.banned_until)){
		ret[sslit("errorCode")] = (int)Msg::ERR_TRANSFER_DEST_BANNED;
		ret[sslit("errorMessage")] = "Destination account is banned";
		return ret;
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(src_info.account_id, ItemTransactionElement::OP_REMOVE, ItemIds::ID_ACCOUNT_BALANCE, delta_balance,
		Events::ItemChanged::R_TRANSFER, src_info.account_id.get(), dst_info.account_id.get(), 0, remarks);
	transaction.emplace_back(dst_info.account_id, ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, delta_balance,
		Events::ItemChanged::R_TRANSFER, src_info.account_id.get(), dst_info.account_id.get(), 0, remarks);
	const auto insufficient_item_id = ItemMap::commit_transaction_nothrow(transaction.data(), transaction.size());
	if(insufficient_item_id){
		ret[sslit("errorCode")] = (int)Msg::ERR_NO_ENOUGH_ACCOUNT_BALANCE;
		ret[sslit("errorMessage")] = "No enough account balance";
		return ret;
	}

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
