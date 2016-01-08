#ifndef EMPERY_CENTER_SINGLETONS_ACTIVATION_CODE_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACTIVATION_CODE_MAP_HPP_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace EmperyCenter {

class ActivationCode;

struct ActivationCodeMap {
	static boost::shared_ptr<ActivationCode> get(const std::string &code);
	static boost::shared_ptr<ActivationCode> require(const std::string &code);

	static void get_all(std::vector<boost::shared_ptr<ActivationCode>> &ret);

	static void insert(const boost::shared_ptr<ActivationCode> &activation_code);
	static void update(const boost::shared_ptr<ActivationCode> &activation_code, bool throws_if_not_exists = true);
	static void remove(const std::string &code) noexcept;

private:
	ActivationCodeMap() = delete;
};

}

#endif
