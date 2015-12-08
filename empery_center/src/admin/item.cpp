#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/err_item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("item/get_all", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto item_box = ItemBoxMap::get(account_uuid);
	if(!item_box){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	std::vector<ItemBox::ItemInfo> items;
	item_box->get_all(items);

	Poseidon::JsonObject temp_object;
	for(auto it = items.begin(); it != items.end(); ++it){
		const auto item_id = it->item_id;
		const auto item_count = it->count;
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)item_id.get());
		temp_object[SharedNts(str, len)] = item_count;
	}
	root[sslit("items")] = std::move(temp_object);

	return Response();
}

ADMIN_SERVLET("item/add", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto item_box = ItemBoxMap::get(account_uuid);
	if(!item_box){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto item_id      = boost::lexical_cast<ItemId>         (params.at("item_id"));
	const auto count_to_add = boost::lexical_cast<boost::uint64_t>(params.at("count_to_add"));
	const auto param1       = boost::lexical_cast<boost::uint64_t>(params.at("param1"));
	const auto param2       = boost::lexical_cast<boost::uint64_t>(params.at("param2"));
	const auto param3       = boost::lexical_cast<boost::uint64_t>(params.at("param3"));

	std::vector<ItemTransactionElement> transaction;
	const auto operation = ItemTransactionElement::OP_ADD;
	transaction.emplace_back(operation, item_id, count_to_add,
		ReasonIds::ID_ADMIN_OPERATION, param1, param2, param3);
	item_box->commit_transaction(transaction);

	return Response();
}

ADMIN_SERVLET("item/remove", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto item_box = ItemBoxMap::get(account_uuid);
	if(!item_box){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	const auto item_id         = boost::lexical_cast<ItemId>         (params.at("item_id"));
	const auto count_to_remove = boost::lexical_cast<boost::uint64_t>(params.at("count_to_remove"));
	const auto saturated       = !params.get("saturated").empty();
	const auto param1          = boost::lexical_cast<boost::uint64_t>(params.at("param1"));
	const auto param2          = boost::lexical_cast<boost::uint64_t>(params.at("param2"));
	const auto param3          = boost::lexical_cast<boost::uint64_t>(params.at("param3"));

	std::vector<ItemTransactionElement> transaction;
	const auto operation = saturated ? ItemTransactionElement::OP_REMOVE_SATURATED : ItemTransactionElement::OP_REMOVE;
	transaction.emplace_back(operation, item_id, count_to_remove,
		ReasonIds::ID_ADMIN_OPERATION, param1, param2, param3);
	item_box->commit_transaction(transaction);

	return Response();
}

}
