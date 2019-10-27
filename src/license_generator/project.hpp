/*
 * Project.hpp
 *
 *  Created on: Oct 20, 2019
 *      Author: Gabriele Contnini
 */

#ifndef SRC_TOOLS_LICENSE_GENERATOR_PROJECT_HPP_
#define SRC_TOOLS_LICENSE_GENERATOR_PROJECT_HPP_

#include <string>

namespace license {

class Project {
private:
	const std::string m_name;
	std::string m_primary_key_file_name;
	std::string m_public_key_file_name;
	std::string m_project_folder;
	std::string m_source_folder;

public:
	Project(const std::string &name, const std::string &primary_key_file_name,
			const std::string &public_key_file_name,
			const std::string &project_folder, const std::string &source_folder);
	int initialize();
	~Project();
};

} /* namespace license */

#endif /* SRC_TOOLS_LICENSE_GENERATOR_PROJECT_HPP_ */
