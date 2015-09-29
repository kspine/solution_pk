#include "precompiled.hpp"
#include "chinapnr_http_session.hpp"
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/mysql/object_base.hpp>
#include "mysql/bill.hpp"
#include "bill_states.hpp"
#include "item_ids.hpp"
#include "singletons/item_map.hpp"
#include "item_transaction_element.hpp"
#include "events/item.hpp"

namespace EmperyPromotion {

ChinaPnRHttpSession::ChinaPnRHttpSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_prefix(std::move(prefix))
{
}
ChinaPnRHttpSession::~ChinaPnRHttpSession(){
}

void ChinaPnRHttpSession::onSyncRequest(const Poseidon::Http::RequestHeaders &requestHeaders, const Poseidon::StreamBuffer & /* entity */){
	PROFILE_ME;
	LOG_EMPERY_PROMOTION(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted ChinaPnR HTTP request from ", getRemoteInfo());

	auto uri = Poseidon::Http::urlDecode(requestHeaders.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_PROMOTION_WARNING("Inacceptable ChinaPnR HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(requestHeaders.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	try {
		const auto &getParams = requestHeaders.getParams;
		if(uri == "settle"){
			const auto &serial = getParams.at("serial");
			LOG_EMPERY_PROMOTION_INFO("Settle bill: serial = ", serial);

			std::vector<boost::shared_ptr<MySql::Promotion_Bill>> objs;
			std::ostringstream oss;
			oss <<"SELECT * FROM `Promotion_Bill` WHERE `serial` = '" <<Poseidon::MySql::StringEscaper(serial) <<"' LIMIT 1";
			MySql::Promotion_Bill::batchLoad(objs, oss.str());
			if(objs.empty()){
				LOG_EMPERY_PROMOTION_DEBUG("No rows returned");
				DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
			}
			const auto obj = std::move(objs.front());

			const auto oldState = obj->get_state();
			if(oldState >= BillStates::ST_CANCELLED){
				LOG_EMPERY_PROMOTION_WARNING("Unexpected bill state: serail = ", serial, ", oldState = ", oldState);
				DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_INTERNAL_SERVER_ERROR);
			}

			obj->set_callbackIp(getRemoteInfo().ip.get());
			obj->set_state(BillStates::ST_CALLBACK_RECEIVED);

			std::vector<ItemTransactionElement> transaction;
			transaction.emplace_back(AccountId(obj->get_accountId()),
				ItemTransactionElement::OP_ADD, ItemIds::ID_ACCOUNT_BALANCE, obj->get_amount());
			ItemMap::commitTransaction(transaction.data(), transaction.size(),
				Events::ItemChanged::R_RECHARGE, obj->get_accountId(), 0, 0, { });
			obj->set_state(BillStates::ST_SETTLED);

			Poseidon::StreamBuffer data;
			data.put("RECV_ORD_ID_");
			data.put(serial);
			send(Poseidon::Http::ST_OK, std::move(data));
		} else {
			DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
		}
	} catch(Poseidon::Http::Exception &){
		throw;
	} catch(std::logic_error &e){
		LOG_EMPERY_PROMOTION_WARNING("std::logic_error thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_BAD_REQUEST);
	} catch(std::exception &e){
		LOG_EMPERY_PROMOTION_WARNING("std::exception thrown: what = ", e.what());
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_INTERNAL_SERVER_ERROR);
	}
}

}
