/*
 * Project.cpp
 *
 *  Created on: Oct 22, 2019
 *      Author: GC
 */

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>

#include "../base_lib/base.h"
#include "../base_lib/crypto_helper.hpp"
#include "../inja/inja.hpp"
#include "project.hpp"

namespace license {
namespace fs = boost::filesystem;
using json = nlohmann::json;
using namespace inja;
using namespace std;

static const constexpr char *const TEMPLATE = "public_key.inja";

static FUNCTION_RETURN check_templates(const string &source_folder) {
	const fs::path source_path(source_folder);
	if (!fs::exists(source_path) || !fs::is_directory(source_path)) {
		throw std::runtime_error(
				"Source directory [" + source_folder
						+ "] does not exist or is not a directory");
	}
	const fs::path templates_path(source_path / "templates");
	if (!fs::exists(templates_path) || !fs::is_directory(templates_path)) {
		throw std::runtime_error(
				string("Templates directory [") + templates_path.string()
						+ "] does not exist or is not a directory");
	}
	const fs::path template_fname(templates_path / TEMPLATE);
	if (!fs::exists(template_fname) || !fs::is_regular_file(template_fname)) {
		throw std::runtime_error(
				string("Templates file [") + template_fname.string()
						+ "] does not exist");
	}
	return FUNC_RET_OK;
}

static const fs::path publicKeyFolder(const fs::path &product_folder,
		const string &product_name) {
	return product_folder / "include" / "licensecc" / product_name;
}

Project::Project(const std::string &name, const std::string &project_folder,
		const std::string &source_folder, bool force_overwrite) :
		m_name(name), m_project_folder(project_folder), m_source_folder(
				source_folder), m_force_overwrite(force_overwrite) {
}

FUNCTION_RETURN Project::initialize() {
	check_templates(m_source_folder);
	const fs::path destinationDir(fs::path(m_project_folder) / m_name);
	const fs::path include_folder(publicKeyFolder(destinationDir, m_name));
	const fs::path publicKeyFile(include_folder / PUBLIC_KEY_INC_FNAME);
	const fs::path privateKeyFile(destinationDir / PRIVATE_KEY_FNAME);
	bool keyFilesExist = false;
	if (fs::exists(destinationDir)) {
		keyFilesExist = fs::exists(destinationDir / PRIVATE_KEY_FNAME);
		keyFilesExist = keyFilesExist && fs::exists(publicKeyFile);
		if (m_force_overwrite && keyFilesExist) {
			keyFilesExist = false;
			fs::remove(destinationDir / PRIVATE_KEY_FNAME);
			fs::remove(publicKeyFile);
		}
		if (!fs::exists(include_folder)) {
			if (!fs::create_directories(include_folder)) {
				throw std::runtime_error(
						"Cannot create public key directory ["
								+ include_folder.string() + "]");
			}
		}
	} else if (!fs::create_directories(destinationDir)
			|| !fs::create_directories(include_folder)) {
		throw std::runtime_error(
				"Cannot create destination directory ["
						+ destinationDir.string() + "]");
	}
	FUNCTION_RETURN result = FUNC_RET_OK;
	if (!keyFilesExist) {
		unique_ptr<CryptoHelper> cryptoHelper(CryptoHelper::getInstance());
		cryptoHelper->generateKeyPair();
		const std::string privateKey = cryptoHelper->exportPrivateKey();
		ofstream ofs(privateKeyFile.c_str());
		ofs << privateKey;
		ofs.close();

		const fs::path templates_path(fs::path(m_source_folder) / "templates");
		Environment env(templates_path.string()+"/",include_folder.string() +"/");

		Template temp = env.parse_template(TEMPLATE);
		json data;
		data["public_key"] = cryptoHelper->exportPublicKey();
		data["product_name"] = m_name;
		env.write(temp, data, PUBLIC_KEY_INC_FNAME);
	} else {
		result = FUNC_RET_NOT_AVAIL;
	}
	return result;
}

Project::~Project() {
}

} /* namespace license */
