/*
 * License.hpp
 *
 *  Created on: Nov 10, 2019
 *      Author: GC
 */

#ifndef SRC_LICENSE_GENERATOR_LICENSE_HPP_
#define SRC_LICENSE_GENERATOR_LICENSE_HPP_
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <iostream>

namespace license {
class License {
private:
	std::string m_private_key;
	std::string m_project_name;

	const bool m_base64;
	const std::string *m_license_fname;
	const std::string m_projectFolder;
	std::map<std::string, std::string> values_map;

	void print_as_ini(std::ostream &a_ostream, const std::string &signature) const;
	const std::string print_for_sign() const;

public:
	License(const std::string *license_fname, const std::string &projectFolder, bool base64 = false);
	void add_parameter(const std::string &param_name, const std::string &param_value);
	void write_license();
	inline virtual ~License() {}
};

} /* namespace license */

#endif /* SRC_LICENSE_GENERATOR_LICENSE_HPP_ */
