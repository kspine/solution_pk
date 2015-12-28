#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../activation_code.hpp"
#include "../singletons/activation_code_map.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

namespace {
	unsigned s_auto_inc = Poseidon::rand32();
}

ADMIN_SERVLET("activation_code/random", root, session, params){
	const auto expiry_duration = boost::lexical_cast<boost::uint64_t>(params.at("expiry_duration"));

	const auto utc_now = Poseidon::get_utc_time();

	std::string code;
	code.resize(11);
	auto seed = utc_now + 6364136223846793005ull * s_auto_inc;
	for(auto it = code.rbegin(); it != code.rend(); ++it){
		*it = static_cast<int>(seed % 26) + 'a';
		seed /= 26;
	}
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
