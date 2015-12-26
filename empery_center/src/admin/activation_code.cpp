#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../activation_code.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("activation_code/random", root, session, params){
	const auto expiry_duration = boost::lexical_cast<boost::uint64_t>(params.at("expiry_duration"));

	static unsigned s_auto_inc = 0;

	const auto utc_now = Poseidon::get_utc_time();
	const auto auto_inc = ++s_auto_inc;

	std::array<char, 11> str;
	auto it = str.rbegin();
	*(it++) = static_cast<int>(auto_inc % 26) + 'a';
	boost::uint64_t val = utc_now;
	for(unsigned i = 0; i < 10; ++i){
		*(it++) = static_cast<int>(val % 26) + 'a';
		val /= 26;
	}
	auto code = std::string(str.data(), str.size());
	auto expiry_time = saturated_add(utc_now, expiry_duration);

	const auto activation_code = boost::make_shared<ActivationCode>(code, utc_now, expiry_time);
	ActivationCodeMap::insert(activation_code);

	root[sslit("code")]        = std::move(code);
	root[sslit("expiry_time")] = expiry_time;

	return Response();
}

ADMIN_SERVLET("activation_code/remove", root, session, params){
	const auto &code = params.at("code");

	const auto activation_code = ActivationCodeMap::get(code);
	if(!activation_code){
		return Response(Msg::ERR_ACTIVATION_CODE_DELETED) <<code;
	}

	activation_code->set_expiry_time(0);

	return Response();
}

}
