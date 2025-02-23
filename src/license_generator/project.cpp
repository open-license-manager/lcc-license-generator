/*
 * Project.cpp
 *
 *  Created on: Oct 22, 2019
 *      Author: GC
 */

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>
#include <algorithm>

#include "../inja/inja.hpp"
#include "../base_lib/base.h"
#include "../base_lib/crypto_helper.hpp"
#include "project.hpp"

namespace license {
namespace fs = boost::filesystem;
using json = nlohmann::json;
using namespace inja;
using namespace std;

static const constexpr char *const TEMPLATE = "public_key.inja";

/*static FUNCTION_RETURN check_templates(const string &source_folder) {
	const fs::path templates_path(source_folder);
	if (!fs::exists(templates_path) || !fs::is_directory(templates_path)) {
		throw std::runtime_error(string("Templates directory [") + templates_path.string() +
								 "] does not exist or is not a directory");
	}
	const fs::path template_fname(templates_path / TEMPLATE);
	if (!fs::exists(template_fname) || !fs::is_regular_file(template_fname)) {
		throw std::runtime_error(string("Templates file [") + template_fname.string() + "] does not exist");
	}
	return FUNC_RET_OK;
}*/

static const string guess_templates_folder(const string &source_folder) {
	fs::path templates_path(source_folder);
	if (!fs::exists(templates_path) || !fs::is_directory(templates_path)) {
		throw std::runtime_error(string("Templates directory [") + templates_path.string() +
								 "] does not exist or is not a directory");
	}
	fs::path template_fname(templates_path / TEMPLATE);
	if (!fs::exists(template_fname) || !fs::is_regular_file(template_fname)) {
		// try to add a /templates
		templates_path = templates_path / "templates";
		fs::path template_fname2(templates_path / TEMPLATE);
		if (!fs::exists(template_fname2) || !fs::is_regular_file(template_fname2)) {
			throw std::runtime_error(string("Templates file [") + template_fname2.string() +
									 "] does not exist. tried also [" + template_fname.string() + "]");
		}
	}
	fs::path normalized = templates_path.lexically_normal();
	return normalized.string();
}
static const fs::path publicKeyFolder(const fs::path &product_folder, const string &product_name) {
	return product_folder / "include" / "licensecc" / product_name;
}

Project::Project(const std::string &name, const std::string &project_folder, const std::string &source_folder,
				 bool force_overwrite)
	: m_name(name),
	  m_project_folder(project_folder),
	  m_templates_folder(guess_templates_folder(source_folder)),
	  m_force_overwrite(force_overwrite) {
	if (name.find('[') != std::string::npos || name.find(']') != std::string::npos ||
		name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
		throw invalid_argument("project name should not contain any of '[ ] / \' characters.");
	}
}

void Project::exportPublicKey(const std::string &include_folder, const std::unique_ptr<CryptoHelper> &cryptoHelper) {
	const fs::path templates_path(m_templates_folder);
	Environment env(templates_path.string() + "/", include_folder + "/");
	Template temp = env.parse_template(TEMPLATE);
	json data;
	const vector<unsigned char> pkey = cryptoHelper->exportPublicKey();
	data["public_key"] = pkey;
	data["public_key_len"] = pkey.size();
	data["product_name"] = m_name;
	env.write(temp, data, PUBLIC_KEY_INC_FNAME);
}

FUNCTION_RETURN Project::initialize() {
	const fs::path destinationDir(fs::path(m_project_folder) / m_name);
	const fs::path include_folder(publicKeyFolder(destinationDir, m_name));
	const fs::path publicKeyFile(include_folder / PUBLIC_KEY_INC_FNAME);
	const fs::path privateKeyFile(destinationDir / PRIVATE_KEY_FNAME);
	bool keyFilesExist = false;
	if (fs::exists(destinationDir)) {
		keyFilesExist = fs::exists(destinationDir / PRIVATE_KEY_FNAME);
		if (m_force_overwrite && keyFilesExist) {
			keyFilesExist = false;
			fs::remove(destinationDir / PRIVATE_KEY_FNAME);
			fs::remove(publicKeyFile);
		}
		if (!fs::exists(include_folder)) {
			if (!fs::create_directories(include_folder)) {
				throw std::runtime_error("Cannot create public key directory [" + include_folder.string() + "]");
			}
		}
	} else if (!fs::create_directories(destinationDir) || !fs::create_directories(include_folder)) {
		throw std::runtime_error("Cannot create destination directory [" + destinationDir.string() + "]");
	}
	FUNCTION_RETURN result = FUNC_RET_OK;
	unique_ptr<CryptoHelper> cryptoHelper(CryptoHelper::getInstance());
	if (keyFilesExist) {
		if (!fs::exists(publicKeyFile)) {
			// how strange, private key was found, but public key is not.
			// Let's regenerate public key
			cryptoHelper->loadPrivateKey_file(privateKeyFile.string());
			exportPublicKey(include_folder.string(), cryptoHelper);
		}
	} else {
		ofstream ofs;
		cryptoHelper->generateKeyPair();
		const std::string privateKey = cryptoHelper->exportPrivateKey();
		const string private_key_file_str = privateKeyFile.string();
		ofs.open(private_key_file_str.c_str(), std::fstream::trunc | std::fstream::binary);
		ofs << privateKey;
		ofs.close();
		exportPublicKey(include_folder.string(), cryptoHelper);
	}
	return result;
}

Project::~Project() {}

} /* namespace license */
