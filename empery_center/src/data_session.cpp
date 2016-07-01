#include "precompiled.hpp"
#include "data_session.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/json.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/string.hpp>

namespace EmperyCenter {

using SerializedData = DataSession::SerializedData;

namespace {
	boost::container::flat_map<std::string, boost::weak_ptr<const SerializedData>> g_servlet_map;
}

DataSession::DataSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_prefix(std::move(prefix))
{
}
DataSession::~DataSession(){
}

boost::shared_ptr<const SerializedData> DataSession::create_servlet(const std::string &name, const Poseidon::JsonArray &data){
	PROFILE_ME;

	auto &weak = g_servlet_map[name];
	if(!weak.expired()){
		LOG_EMPERY_CENTER_ERROR("Duplicate data HTTP servlet: name = ", name);
		DEBUG_THROW(Exception, sslit("Duplicate data HTTP servlet"));
	}

	auto utf8_string = data.dump();
	if(!Poseidon::is_valid_utf8_string(utf8_string)){
		DEBUG_THROW(Exception, sslit("Invalid UTF-8 string"));
	}
	auto md5 = Poseidon::md5_hash(utf8_string);
	auto md5_as_hex = Poseidon::Http::hex_encode(md5.data(), md5.size());

	auto servlet = boost::make_shared<SerializedData>();
	servlet->utf8_string = std::move(utf8_string);
	servlet->md5_entry = Poseidon::JsonElement(name).dump() + ':' + Poseidon::JsonElement(std::move(md5_as_hex)).dump();
	weak = servlet;
	return std::move(servlet);
}
boost::shared_ptr<const SerializedData> DataSession::get_servlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_servlet_map.find(uri);
	if(it == g_servlet_map.end()){
		return { };
	}
	auto servlet = it->second.lock();
	if(!servlet){
		g_servlet_map.erase(it);
		return { };
	}
	return servlet;
}

void DataSession::on_sync_request(Poseidon::Http::RequestHeaders request_headers, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted data HTTP request from ", get_remote_info());

	auto uri = Poseidon::Http::url_decode(request_headers.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_CENTER_WARNING("Inacceptable data HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(request_headers.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	Poseidon::OptionalMap headers;
	headers.set(sslit("Content-Type"), "application/json; charset=utf-8");
	Poseidon::StreamBuffer contents;
	if(!uri.empty()){
		const auto servlet = get_servlet(uri);
		if(!servlet){
			LOG_EMPERY_CENTER_WARNING("No servlet available: uri = ", uri);
			DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
		}
		contents.put(servlet->utf8_string);
	} else {
		contents.put('{');
		for(auto it = g_servlet_map.begin(); it != g_servlet_map.end(); ++it){
			const auto servlet = it->second.lock();
			if(!servlet){
				continue;
			}
			contents.put(servlet->md5_entry);
			contents.put(',');
		}
		if(contents.back() == ','){
			contents.unput();
		}
		contents.put('}');
	}
	send(Poseidon::Http::ST_OK, std::move(headers), std::move(contents));
}

}
