#include "license-generator.h"

#include <stddef.h>
#include <stdlib.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <build_properties.h>
#include "../base_lib/base64.h"
#include "../base_lib/crypto_helper.hpp"

//namespace fs = boost::filesystem;
//namespace bt = boost::posix_time;
namespace po = boost::program_options;

using namespace std;

namespace license {


static po::options_description configureProgramOptions() {
	po::options_description common("General options");
	common.add_options()("help,h", "print help message and exit.") //
	("verbose,v", "print more information.") //

	("output,o", po::value<string>(), "Output file name. If not specified the "
			"license will be printed to standard output");
	po::options_description licenseGeneration("License Generation");
	licenseGeneration.add_options()("private_key,p", po::value<string>(),
			"Specify an alternate file for the primary key to be used. "
					"If not specified the internal primary key will be used.")(
			"begin_date,b", po::value<string>(),
			"Specify the start of the validity for this license. "
					" Format YYYYMMDD. If not specified defaults to today")(
			"expire_date,e", po::value<string>(),
			"Specify the expire date for this license. "
					" Format YYYYMMDD. If not specified the license won't expire")(
			"client_signature,s", po::value<string>(),
			"The signature of the pc that requires the license. "
					"It should be in the format XXXX-XXXX-XXXX-XXXX."
					" If not specified the license "
					"won't be linked to a specific pc.")("start_version,t",
			po::value<unsigned int>()->default_value(0
			/*FullLicenseInfo.UNUSED_SOFTWARE_VERSION*/, "All Versions"),
			"Specify the first version of the software this license apply to.")(
			"end_version,n", po::value<unsigned int>()->default_value(0
			/*FullLicenseInfo.UNUSED_SOFTWARE_VERSION*/, "All Versions"),
			"Specify the last version of the software this license apply to.")(
			"extra_data,x", po::value<string>(),
			"Specify extra data to be included into the license");
	po::options_description visibleOptions;
	visibleOptions.add(common).add(licenseGeneration);
	return visibleOptions;
}
/*
vector<FullLicenseInfo> LicenseGenerator::parseLicenseInfo(
		const po::variables_map &vm) {
	string begin_date = FullLicenseInfo::UNUSED_TIME;
	string end_date = FullLicenseInfo::UNUSED_TIME;
	if (vm.count("expire_date")) {
		const std::string dt_end = vm["expire_date"].as<string>();
		try {
			end_date = normalize_date(dt_end);
			char curdate[20];
			time_t curtime = time(nullptr);
			strftime(curdate, 20, "%Y-%m-%d", localtime(&curtime));
			begin_date.assign(curdate);
		} catch (const invalid_argument &e) {
			cerr << endl << "End date not recognized: " << dt_end
					<< " Please enter a valid date in format YYYYMMDD" << endl;
			exit(2);
		}
	}
	if (vm.count("begin_date")) {
		const std::string begin_date_str = vm["begin_date"].as<string>();
		try {
			begin_date = normalize_date(begin_date_str);
		} catch (invalid_argument &e) {
			cerr << endl << "Begin date not recognized: " << begin_date_str
					<< " Please enter a valid date in format YYYYMMDD" << endl;
			//print_usage(vm);
			exit(2);
		}
	}
	string client_signature = "";
	if (vm.count("client_signature")) {
		client_signature = vm["client_signature"].as<string>();
		cout << "cli sig:" << client_signature;
		regex e(
				"[A-Za-z0-9\\+/]{4}-[A-Za-z0-9\\+/]{4}-[A-Za-z0-9\\+/]{4}-[A-Za-z0-9\\+/]{4}");
		cout << "\nregex:";
		if (!regex_match(client_signature, e)) {
			cerr << endl << "Client signature not recognized: "
					<< client_signature
					<< " Please enter a valid signature in format XXXX-XXXX-XXXX-XXXX"
					<< endl;
			exit(2);
		}
	}
	string extra_data = "";
	if (vm.count("extra_data")) {
		extra_data = vm["extra_data"].as<string>();
	}
	unsigned int from_sw_version = vm["start_version"].as<unsigned int>();
	unsigned int to_sw_version = vm["end_version"].as<unsigned int>();
	if (vm.count("product") == 0) {
		cerr << endl << "Parameter [product] not found. " << endl;
		exit(2);
	}
	vector<string> products = vm["product"].as<vector<string>>();
	vector<FullLicenseInfo> licInfo;
	licInfo.reserve(products.size());
	for (auto it = products.begin(); it != products.end(); it++) {
		if (boost::algorithm::trim_copy(*it).length() > 0) {
			licInfo.push_back(
					FullLicenseInfo("", *it, "", PROJECT_INT_VERSION,
							begin_date, end_date, client_signature,
							from_sw_version, to_sw_version, extra_data));
		}
	}
	return licInfo;
}
*/
void LicenseGenerator::generateAndOutputLicenses(const po::variables_map &vm,
		ostream &outputFile) {
/*	vector<FullLicenseInfo> licenseInfo = parseLicenseInfo(vm);
	const unique_ptr<CryptoHelper> helper = CryptoHelper::getInstance();
	const unsigned char pkey[] = PRIVATE_KEY;
	const size_t len = sizeof(pkey);
	for (auto it = licenseInfo.begin(); it != licenseInfo.end(); ++it) {
		const string license = it->printForSign();
		const string signature = helper->signString((const void*) pkey, len,
				license);
		it->license_signature = signature;
		it->printAsIni(outputFile);
	}*/
}

void LicenseGenerator::generateB64Licenses(const po::variables_map &vm,
		ostream &outputFile) {
	std::ostringstream tempStream;
	generateAndOutputLicenses(vm, tempStream);

	std::string str = tempStream.str();
	const char *chr = str.c_str();
	size_t finalLenght;
	string encoded = base64(chr, str.length());
	outputFile.write(encoded.c_str(), encoded.length());
}

int LicenseGenerator::generateLicense(int argc, const char **argv) {

	return 0;
}

const std::string formats[] = { "%4u-%2u-%2u", "%4u/%2u/%2u", "%4u%2u%2u" };
const size_t formats_n = 3;

string LicenseGenerator::normalize_date(const std::string &sDate) {
	if (sDate.size() < 8)
		throw invalid_argument("Date string too small for known formats");
	unsigned int year, month, day;
	bool found = false;
	for (size_t i = 0; i < formats_n && !found; ++i) {
		const int chread = sscanf(sDate.c_str(), formats[i].c_str(), &year,
				&month, &day);
		if (chread == 3) {
			found = true;
			break;
		}
	}
	if (!found)
		throw invalid_argument("Date string did not match a known format");
	ostringstream oss;
	oss << year << "-" << setfill('0') << std::setw(2) << month << "-"
			<< setfill('0') << std::setw(2) << day;
	return oss.str();
}
}
