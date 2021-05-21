/*
 * Project.hpp
 *
 *  Created on: Oct 20, 2019
 *      Author: Gabriele Contnini
 */

#ifndef SRC_TOOLS_LICENSE_GENERATOR_PROJECT_HPP_
#define SRC_TOOLS_LICENSE_GENERATOR_PROJECT_HPP_

#include "../base_lib/base.h"
#include "../base_lib/crypto_helper.hpp"
#include <boost/optional.hpp>
#include <string>

namespace license {

class Project {
private:
	const std::string m_name;
	boost::optional<std::string> m_primary_key_file_name(boost::optional<std::string>());
	boost::optional<std::string> m_public_key_file_name(boost::optional<std::string>());
	void exportPublicKey(const std::string &include_folder, const std::unique_ptr<CryptoHelper> &cryptoHelper);

	std::string m_project_folder;
	std::string m_templates_folder;
	const bool m_force_overwrite;

public:
	Project(const std::string &name, const std::string &project_folder, const std::string &source_folder,
			const bool force_overwrite = false);
	FUNCTION_RETURN initialize();
	~Project();
};

} /* namespace license */

#endif /* SRC_TOOLS_LICENSE_GENERATOR_PROJECT_HPP_ */
