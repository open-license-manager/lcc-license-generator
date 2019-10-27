/*
 * Project.cpp
 *
 *  Created on: Oct 22, 2019
 *      Author: devel
 */

#include "project.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>

namespace license {
namespace fs = boost::filesystem;
using namespace std;

Project::Project(const std::string &name,
		const std::string &primary_key_file_name,
		const std::string &public_key_file_name,
		const std::string &project_folder, const std::string &source_folder) :
		m_name(name), m_primary_key_file_name(primary_key_file_name), m_public_key_file_name(
				public_key_file_name), m_project_folder(project_folder), m_source_folder(
				source_folder) {
}

int Project::initialize() {
	fs::path destinationDir(fs::path(m_project_folder) / m_name);
	if (!fs::exists(m_source_folder) || !fs::is_directory(m_source_folder)) {
		throw std::runtime_error(
				"Source directory [" + m_source_folder
						+ "] does not exist or is not a directory");
	}
	if (fs::exists(destinationDir)) {
		throw std::runtime_error(
				"Destination directory [" + destinationDir.string()
						+ "] already exists");
	}
	if (!fs::create_directory(destinationDir)) {
		throw std::runtime_error(
				"Cannot create destination directory ["
						+ destinationDir.string() + "]");
	}
	fs::path sourceDir(m_source_folder);
	for (const auto &dirEnt : fs::recursive_directory_iterator { sourceDir }) {
		const auto &path = dirEnt.path();
		auto relativePathStr = path.string();
		boost::replace_first(relativePathStr, sourceDir.string(), "");
		fs::copy(path, destinationDir / relativePathStr);
	}
	return 0;
}

Project::~Project() {
	// TODO Auto-generated destructor stub
}

} /* namespace license */
