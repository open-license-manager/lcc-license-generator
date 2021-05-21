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

static const unordered_set<string> NO_OUTPUT_PARAM = {
	PARAM_BASE64,		  PARAM_LICENSE_OUTPUT, PARAM_FEATURE_NAMES,
	PARAM_PROJECT_FOLDER, PARAM_PRIMARY_KEY,	PARAM_MAGIC_NUMBER,
};

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
	if (!found) throw invalid_argument("Date [" + sDate + "] did not match a known format. try YYYY-MM-DD");
	ostringstream oss;
	oss << year << "-" << setfill('0') << std::setw(2) << month << "-" << setfill('0') << std::setw(2) << day;
	return oss.str();
}

static const string normalize_project_path(const string &project_path) {
	const fs::path rproject_path(project_path);
	if (!fs::exists(rproject_path) || !fs::is_directory(rproject_path)) {
		throw logic_error("Path " + project_path + " doesn't exist or is not a directory.");
	}
	fs::path normalized;
	const string rproject_path_str = rproject_path.string();
	if (rproject_path.string() == ".") {
		normalized = fs::current_path();
		// sometimes is_relative fails under wine: a linux path is taken for a relative path.
		normalized = fs::canonical(fs::current_path() / rproject_path);
	} else {
		normalized = fs::canonical(rproject_path);
	}
	return normalized.string();
}

static void create_license_path(const string &license_file_name) {
	const fs::path license_name(license_file_name);
	fs::path parentPath = license_name.parent_path();
	if (!parentPath.empty()) {
		if (!fs::exists(parentPath)) {
			if (!fs::create_directories(parentPath)) {
				throw runtime_error("Cannot create licenses directory [" + parentPath.string() + "]");
			}
		} else if (fs::is_regular_file(parentPath)) {
			throw runtime_error("trying to create folder [" + parentPath.string() +
								"] but there is a file with the same name. ");
		}
	}
}

static const string print_for_sign(const string &feature_name, const CSimpleIniA::TKeyVal *section) {
	stringstream buf;
	buf << boost::to_upper_copy(feature_name);
	for (auto it = section->begin(); it != section->end(); it++) {
		string key(it->first.pItem);
		if (key != LICENSE_SIGNATURE) {
			buf << boost::algorithm::trim_copy(key) << boost::algorithm::trim_copy(string(it->second));
		}
	}
	return buf.str();
}

License::License(const std::string *licenseName, const std::string &project_folder, bool base64)
	: m_base64(base64), m_license_fname(licenseName), m_project_folder(normalize_project_path(project_folder)) {
	fs::path proj_folder(m_project_folder);
	// default feature = project name
	m_feature_names = proj_folder.filename().string();
	m_private_key = (proj_folder / PRIVATE_KEY_FNAME).string();
}

void License::write_license() {
	ofstream license_stream;
	ostream *output_license;
	CSimpleIniA ini;
	if (m_license_fname == nullptr) {
		output_license = &cout;
	} else {
		ifstream previous_license(*m_license_fname);
		if (previous_license.is_open()) {
			SI_Error error = ini.LoadData(previous_license);
			if (error != SI_Error::SI_OK) {
				throw runtime_error(
					"License file existing, but there were errors in loading it. Is it a license file?");
			}
		} else {
			// new license
			create_license_path(*m_license_fname);
		}
		output_license = &license_stream;
		license_stream.open(*m_license_fname, ios::trunc | ios::binary);
		if (!license_stream.is_open()) {
			throw runtime_error("Can not create file [" + *m_license_fname + "].");
		}
	}

	const string features = boost::to_upper_copy(m_feature_names);
	vector<string> feature_v;
	boost::algorithm::split(feature_v, features, boost::is_any_of(","));
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->loadPrivateKey_file(m_private_key);

	for (const string feature : feature_v) {
		ini.SetLongValue(feature.c_str(), "lic_ver", LICENSE_FILE_VERSION);
		for (auto it : values_map) {
			ini.SetValue(feature.c_str(), it.first.c_str(), it.second.c_str());
		}
		const CSimpleIniA::TKeyVal *section = ini.GetSection(feature.c_str());
		string license_for_sign = print_for_sign(feature, section);
		const string signature = crypto->signString(license_for_sign);
		ini.SetValue(feature.c_str(), LICENSE_SIGNATURE, signature.c_str());
	}
	ini.Save(*output_license, true);
}

// TODO better validation on the input parameters
// TODO, split this code in multiple classes
void License::add_parameter(const std::string &param_name, const std::string &param_value) {
	if (NO_OUTPUT_PARAM.find(param_name) == NO_OUTPUT_PARAM.end()) {
		if (param_name.find("date") != std::string::npos || param_name.find(PARAM_EXPIRY_DATE) != std::string::npos ||
			param_name.find(PARAM_BEGIN_DATE) != std::string::npos) {
			values_map[param_name] = normalize_date(param_value);
		} else if (param_name.find("version") != std::string::npos) {
			if (param_value != "0") {
				values_map[param_name] = param_value;
			}
		} else {
			values_map[param_name] = param_value;
		}
	} else if (PARAM_FEATURE_NAMES == param_name) {
		m_feature_names = param_value;
		if (m_feature_names.find('[') != std::string::npos || m_feature_names.find(']') != std::string::npos ||
			m_feature_names.find('/') != std::string::npos || m_feature_names.find('\\') != std::string::npos) {
			throw invalid_argument(
				string("feature name should not contain any of '[ ] / \' characters. Parameter " PARAM_FEATURE_NAMES
					   "value :") +
				param_name);
		}
	} else if (PARAM_PRIMARY_KEY == param_name) {
		if (!fs::exists(param_value)) {
			cerr << "Primary key " << param_value << " not found." << endl;
			throw logic_error("Primary key [" + param_value + "] not found");
		}
		m_private_key = param_value;
	} else if (PARAM_LICENSE_OUTPUT == param_name || PARAM_PROJECT_FOLDER == param_name) {
		// just ignore
	} else {
		throw logic_error(param_name + " not recognized");
	}
}
} /* namespace license */
