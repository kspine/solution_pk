#include "precompiled.hpp"
#include "data_session.hpp"
#include <poseidon/http/verbs.hpp>
#include <poseidon/http/status_codes.hpp>
#include <poseidon/http/exception.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/job_base.hpp>
#include <poseidon/json.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/string.hpp>

namespace EmperyCenter {

using SerializedData = DataSession::SerializedData;

namespace {
	std::map<std::string, boost::weak_ptr<const SerializedData>> g_servletMap;
}

DataSession::DataSession(Poseidon::UniqueFile socket, std::string prefix)
	: Poseidon::Http::Session(std::move(socket))
	, m_prefix(std::move(prefix))
{
}
DataSession::~DataSession(){
}

boost::shared_ptr<const SerializedData> DataSession::createServlet(const std::string &name, const Poseidon::JsonArray &data){
	PROFILE_ME;

	auto utf8String = data.dump();
	if(!Poseidon::isValidUtf8String(utf8String)){
		DEBUG_THROW(Exception, sslit("Invalid UTF-8 string"));
	}
	auto md5 = Poseidon::md5Hash(utf8String);
	auto md5AsHex = Poseidon::Http::hexEncode(md5.data(), md5.size());

	auto servlet = boost::make_shared<SerializedData>();
	servlet->utf8String = std::move(utf8String);
	servlet->md5Entry = Poseidon::JsonElement(name).dump() + ':' + Poseidon::JsonElement(std::move(md5AsHex)).dump();
	if(!g_servletMap.insert(std::make_pair(name, servlet)).second){
		LOG_EMPERY_CENTER_ERROR("Duplicate data HTTP servlet: name = ", name);
		DEBUG_THROW(Exception, sslit("Duplicate data HTTP servlet"));
	}
	return std::move(servlet);
}
boost::shared_ptr<const SerializedData> DataSession::getServlet(const std::string &uri){
	PROFILE_ME;

	const auto it = g_servletMap.find(uri);
	if(it == g_servletMap.end()){
		return { };
	}
	auto servlet = it->second.lock();
	if(!servlet){
		g_servletMap.erase(it);
		return { };
	}
	return servlet;
}

void DataSession::onSyncRequest(Poseidon::Http::RequestHeaders requestHeaders, Poseidon::StreamBuffer /* entity */){
	PROFILE_ME;
	LOG_EMPERY_CENTER(Poseidon::Logger::SP_MAJOR | Poseidon::Logger::LV_INFO,
		"Accepted data HTTP request from ", getRemoteInfo());

	auto uri = Poseidon::Http::urlDecode(requestHeaders.uri);
	if((uri.size() < m_prefix.size()) || (uri.compare(0, m_prefix.size(), m_prefix) != 0)){
		LOG_EMPERY_CENTER_WARNING("Inacceptable data HTTP request: uri = ", uri, ", prefix = ", m_prefix);
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
	}
	uri.erase(0, m_prefix.size());

	if(requestHeaders.verb != Poseidon::Http::V_GET){
		DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_METHOD_NOT_ALLOWED);
	}

	Poseidon::OptionalMap headers;
	headers.set(sslit("Content-Type"), "application/json; charset=utf-8");
	Poseidon::StreamBuffer contents;
	if(!uri.empty()){
		const auto servlet = getServlet(uri);
		if(!servlet){
			LOG_EMPERY_CENTER_WARNING("No servlet available: uri = ", uri);
			DEBUG_THROW(Poseidon::Http::Exception, Poseidon::Http::ST_NOT_FOUND);
		}
		contents.put(servlet->utf8String);
	} else {
		contents.put('{');
		for(auto it = g_servletMap.begin(); it != g_servletMap.end(); ++it){
			const auto servlet = it->second.lock();
			if(!servlet){
				continue;
			}
			contents.put(servlet->md5Entry);
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
