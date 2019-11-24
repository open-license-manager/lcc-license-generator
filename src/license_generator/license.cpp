/*
 * License.cpp
 *
 *  Created on: Nov 10, 2019
 *      Author: GC
 */
#define SI_SUPPORT_IOSTREAMS

#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "../ini/SimpleIni.h"
#include "../base_lib/crypto_helper.hpp"
#include "../base_lib/base.h"
#include "license.hpp"

namespace license {
using namespace std;
namespace fs = boost::filesystem;

static const unordered_set<string> NO_OUTPUT_PARAM = {PARAM_BASE64, PARAM_LICENSE_NAME, PARAM_PRODUCT_NAME,
													  PARAM_PROJECT_FOLDER, PARAM_PRIMARY_KEY};

const std::string formats[] = {"%4u-%2u-%2u", "%4u/%2u/%2u", "%4u%2u%2u"};
const size_t formats_n = 3;

static const string normalize_date(const std::string &sDate) {
	if (sDate.size() < 8) throw invalid_argument("Date string too small for known formats");
	unsigned int year, month, day;
	bool found = false;
	for (size_t i = 0; i < formats_n && !found; ++i) {
		const int chread = sscanf(sDate.c_str(), formats[i].c_str(), &year, &month, &day);
		if (chread == 3) {
			found = true;
			break;
		}
	}
	if (!found) throw invalid_argument("Date string did not match a known format");
	ostringstream oss;
	oss << year << "-" << setfill('0') << std::setw(2) << month << "-" << setfill('0') << std::setw(2) << day;
	return oss.str();
}

static const string normalize_project_path(const string &project_path) {
	fs::path rproject_path(project_path);
	if (!fs::exists(rproject_path) || !fs::is_directory(rproject_path)) {
		throw logic_error("Path " + project_path + " doesn't exist or is not a directory.");
	}
	fs::path normalized;
	if (rproject_path.filename().string() == ".") {
		normalized = fs::current_path();
		// sometimes is_relative fails under wine. a linux path is taken for a relative path.
	} else if (rproject_path.is_relative() && !rproject_path.string().at(0) == '/') {
		normalized = fs::canonical(fs::current_path() / rproject_path);
	} else {
		normalized = rproject_path.string();
	}
	return normalized.string();
}

static const string print_for_sign(const string &project, const std::map<std::string, std::string> &values_map) {
	stringstream buf;
	buf << boost::to_upper_copy(project);
	for (auto &it : values_map) {
		buf << boost::algorithm::trim_copy(it.first) << boost::algorithm::trim_copy(it.second);
	}
	// cout << "!!! -----:" << buf.str() << endl;
	return buf.str();
}

License::License(const std::string &licenseName, const std::string &project_folder, bool base64)
	: m_base64(base64), m_licenseName(licenseName), m_projectFolder(normalize_project_path(project_folder)) {
	fs::path cur_path(m_projectFolder);
	m_project_name = cur_path.filename().string();
	m_private_key = (cur_path / PRIVATE_KEY_FNAME).string();
}

void License::printAsIni(ostream &a_ostream, const string &signature) const {
	CSimpleIniA ini;
	string result;
	const string product = boost::to_upper_copy(m_project_name);
	CSimpleIniA::StreamWriter sw(a_ostream);
	ini.SetLongValue(product.c_str(), "lic_ver", 200);
	for (auto it : values_map) {
		ini.SetValue(product.c_str(), it.first.c_str(), it.second.c_str());
	}
	ini.SetValue(product.c_str(), "sig", signature.c_str());
	ini.Save(sw, true);
}

void License::write_license() {
	const fs::path project_folder(m_projectFolder);
	fs::path license_folder(project_folder / "licenses");
	const fs::path license_name(m_licenseName + ".lic");
	if (!fs::exists(license_folder)) {
		if (!fs::create_directories(license_folder)) {
			throw runtime_error("Cannot create licenses directory [" + license_folder.string() + "]");
		}
	}
	if (!license_name.parent_path().empty()) {
		fs::path license_final_path(license_folder / license_name.parent_path());
		if (!fs::create_directories(license_final_path)) {
			throw runtime_error("Cannot create licenses directory [" + license_final_path.string() + "]");
		}
		license_folder = license_final_path;
	}

	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->loadPrivateKey_file(m_private_key);

	string license_for_sign = print_for_sign(m_project_name, values_map);
	string signature = crypto->signString(license_for_sign);

	ofstream license_stream;
	fs::path license_path(license_folder / license_name.filename());
	license_stream.open(license_path.string().c_str(), ios::trunc | ios::binary);
	printAsIni(license_stream, signature);
	license_stream.close();
}

// TODO better validation on the input parameters
// TODO, split this code in multiple classes
void License::add_parameter(const std::string &param_name, const std::string &param_value) {
	if (NO_OUTPUT_PARAM.find(param_name) == NO_OUTPUT_PARAM.end()) {
		if (param_name.find("date") != std::string::npos) {
			values_map[param_name] = normalize_date(param_value);
		} else if (param_name.find("version") != std::string::npos) {
			if (param_value != "0") {
				values_map[param_name] = param_value;
			}
		} else {
			values_map[param_name] = param_value;
		}
	} else if (PARAM_PRODUCT_NAME == param_name) {
		m_project_name = param_value;
	} else if (PARAM_PRIMARY_KEY == param_name) {
		if (!fs::exists(param_value)) {
			cerr << "Primary key " << param_value << " not found." << endl;
			throw new logic_error("Primary key [" + param_value + "] not found");
		}
		m_private_key = param_value;
	}
}
} /* namespace license */
