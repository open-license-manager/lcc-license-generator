/*
 * Project.cpp
 *
 *  Created on: Oct 22, 2019
 *      Author: devel
 */

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>

#include "../base_lib/crypto_helper.hpp"
#include "../base_lib/base.h"
#include "product.hpp"

namespace license {
namespace fs = boost::filesystem;
using namespace std;

Project::Project(const std::string &name,
		const std::string &project_folder, const std::string &source_folder,
		bool force_overwrite) :
		m_name(name), m_project_folder(project_folder), m_source_folder(
				source_folder), m_force_overwrite(force_overwrite) {
}

static const constexpr char *const TEMPLATES[] = { "public_key.inja",
		"datatypes.inja" };

static FUNCTION_RETURN check_templates(const string &source_folder,
		fs::path templates_arr[]) {
	fs::path source_path(source_folder);
	if (!fs::exists(source_path) || !fs::is_directory(source_path)) {
		throw std::runtime_error(
				"Source directory [" + source_folder
						+ "] does not exist or is not a directory");
	}
	fs::path templates_path(source_path / "templates");
	if (!fs::exists(templates_path) || !fs::is_directory(templates_path)) {
		throw std::runtime_error(
				string("Templates directory [") + templates_path.string()
						+ "] does not exist or is not a directory");
	}
	for (string file : TEMPLATES) {

	}
	return FUNC_RET_OK;
}

static const fs::path publicKeyFolder(const fs::path &product_folder,
		const string &product_name) {
	return product_folder / "include" / "licensecc" / product_name;
}

int Project::initialize() {
	check_templates(m_source_folder, nullptr);
	const fs::path destinationDir(fs::path(m_project_folder) / m_name);
	const fs::path public_key_folder(publicKeyFolder(destinationDir, m_name));
	const fs::path publicKeyFile(public_key_folder / PUBLIC_KEY_INC_FNAME);
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
		if (!fs::exists(public_key_folder)) {
			if (!fs::create_directories(public_key_folder)) {
				throw std::runtime_error(
						"Cannot create public key directory ["
								+ public_key_folder.string() + "]");
			}
		}
	} else if (!fs::create_directories(destinationDir)
			|| !fs::create_directories(public_key_folder)) {
		throw std::runtime_error(
				"Cannot create destination directory ["
						+ destinationDir.string() + "]");
	}
	int result;
	if (!keyFilesExist) {
		unique_ptr<CryptoHelper> cryptoHelper(
				CryptoHelper::getInstance(m_name));
		cryptoHelper->generateKeyPair();
		const std::string privateKey = cryptoHelper->exportPrivateKey();
		ofstream ofs(privateKeyFile.c_str());
		ofs << privateKey;
		ofs.close();
	} else {
		result = -1;
	}
	return result;
}

Project::~Project() {
	// TODO Auto-generated destructor stub
}

} /* namespace license */
