/*
 * command_line-parser.cpp
 *
 *  Created on: Oct 20, 2019
 *      Author: Gabriele Contini
 */

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
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/config.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <build_properties.h>

#include "../base_lib/base64.h"
#include "command_line-parser.hpp"
#include "license.hpp"
#include "project.hpp"

namespace license {
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;

static void printHelpHeader(const char *prog_name) {
	cout << endl;
	cout << fs::path(prog_name).filename().string() << " Version " << PROJECT_VERSION << ". Usage:" << endl;
}

static void printBasicHelp(const char *prog_name) {
	printHelpHeader(prog_name);
	cout << fs::path(prog_name).filename().string() << " [command] [options]" << endl;
	cout << " available commands: \"project initialize\", \"project list\", \"license issue\", \"license list\""
		 << endl;
	cout << " to see specific command options type: " << prog_name << " [command] --help" << endl << endl;
}

CommandLineParser::CommandLineParser() {}

CommandLineParser::~CommandLineParser() {}

static bool rerunBoostPO(const po::parsed_options &parsed, const po::options_description &project_desc,
						 po::variables_map &vm, const char **argv, const std::string &command_for_logging,
						 const po::options_description &global) {
	// Collect all the unrecognized options from the first pass. This will include the
	// (positional) command name, so we need to erase that.
	// Parse again...
	bool cont = false;
	std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
	opts.erase(opts.begin());
	po::store(po::command_line_parser(opts).options(project_desc).run(), vm);
	if (vm.find("help") == vm.end()) {
		try {
			po::notify(vm);
			cont = true;
		} catch (std::exception &e) {
			printHelpHeader(argv[0]);
			cout << argv[0] << command_for_logging << " [options]" << endl;
			global.print(cout);
			project_desc.print(cout);
			std::cerr << "Error: " << e.what() << endl;
		}
	} else {
		printHelpHeader(argv[0]);
		cout << argv[0] << command_for_logging << " [options]" << endl;
		global.print(cout);
		project_desc.print(cout);
	}
	return cont;
}

static void initializeProject(const po::parsed_options &parsed, po::variables_map &vm, const char **argv,
							  const po::options_description &global) {
	po::options_description project_desc("project init options");
	std::string project_name;
	boost::optional<std::string> primary_key;
	boost::optional<std::string> public_key;
	std::string project_folder;
	std::string templates_folder;
	project_desc.add_options()  //
		("project-name,n", po::value<std::string>(&project_name)->required(), "New project name (required).")  //
		(PARAM_PRIMARY_KEY, po::value<boost::optional<std::string>>(&primary_key),
		 "use externally generated primary key, public key must also be specified.")  //
		("public-key", po::value<boost::optional<std::string>>(&public_key),
		 "Use externally generated public key, private key must also be specified.")  //
		("projects-folder,p", po::value<std::string>(&project_folder)->default_value("."),  //
		 "path to where all the projects configurations are stored.")  //
		("templates,t", po::value<std::string>(&templates_folder)->default_value("."),
		 "path to the templates folder.")  //
		("help", "Print this help.");  //
	if (rerunBoostPO(parsed, project_desc, vm, argv, "project init", global)) {
		// cout << templates_folder.is_initialized() << endl;
		Project project(project_name, project_folder, templates_folder);
		project.initialize();
	}
}

static void issueLicense(const po::parsed_options &parsed, po::variables_map &vm, const char **argv,
						 const po::options_description &global) {
	po::options_description license_desc("license issue options");
	string license_name;
	string *license_name_ptr = nullptr;
	string project_folder;
	// string output;
	unsigned int magic_num = 0;
	bool base64 = false;
	license_desc.add_options()  //
		(PARAM_BASE64 ",b", po::bool_switch(&base64),
		 "Encode license as base64 for inclusion in environment variables.")  //
		(PARAM_BEGIN_DATE, po::value<string>(),
		 "Specify the start of the validity for this license. "
		 " Format YYYYMMDD. If not specified defaults to today")  //
		(PARAM_EXPIRY_DATE ",e", po::value<string>(),
		 "Specify the expire date for this license. "
		 " Format YYYYMMDD. If not specified the license won't expire")  //
		(PARAM_CLIENT_SIGNATURE ",s", po::value<string>(),
		 "The signature of the hardware that requires the license. It should be in the format XXXX-XXXX-XXXX-XXXX."
		 " If not specified the license won't be linked to a specific hardware (eg. demo license).")  //
		(PARAM_LICENSE_OUTPUT ",o", po::value<string>(&license_name),
		 "License output file name. May contain / that will be interpreded as subfolders.")  //
		(PARAM_FEATURE_NAMES ",f", po::value<boost::optional<std::string>>(),
		 "Feature names: comma separate list of project features to enable. if not specified will be taken as project "
		 "name.")  //
		(PARAM_PRIMARY_KEY, po::value<string>(), "Primary key location, in case it is not in default folder")  //
		(PARAM_PROJECT_FOLDER ",p", po::value<string>(&project_folder)->default_value("."),
		 "path to where project configurations and licenses are stored.")  //
		(PARAM_VERSION_FROM, po::value<string>()->default_value("0", "All Versions"),
		 "Specify the first version of the software this license apply to.")  //
		(PARAM_VERSION_TO, po::value<string>()->default_value("0", "All Versions"),  //
		 "Specify the last version of the software this license apply to.")  //
		(PARAM_EXTRA_DATA ",x", po::value<string>(), "Specify extra data to be included into the license")  //
		("help,h", "Print this help.");  //
	if (rerunBoostPO(parsed, license_desc, vm, argv, "license issue", global)) {
		if (!license_name.empty()) {
			license_name_ptr = &license_name;
		}
		License license(license_name_ptr, project_folder, base64);
		for (const auto &it : vm) {
			auto &value = it.second.value();
			if (it.first != "command" && it.first != "subargs" && it.first != "base64") {
				if (auto v = boost::any_cast<std::string>(&value)) {
					license.add_parameter(it.first, *v);
				} else if (auto v = boost::any_cast<boost::optional<std::string>>(value)) {
					license.add_parameter(it.first, *v);
				} else {
					std::cout << it.first << "not recognized value error" << endl;
				}
			}
		}
		try {
			license.write_license();
			cout << "License written " << endl;
		} catch (exception &ex) {
			cerr << "License writing error: " << ex.what() << endl;
		}
	}
}

/** method used in tests for have a quick signature of a piece of data */

static void test_sign(const po::parsed_options &parsed, po::variables_map &vm, const char **argv,
					  const po::options_description &global) {
	po::options_description license_desc("license issue options");
	string private_key_file;
	string data;
	string outputFile;
	license_desc.add_options()  //
		("data,d", po::value<string>(&data)->required(), "Data to be signed")  //
		(PARAM_PRIMARY_KEY ",p", po::value<string>(&private_key_file)->required(), "Primary key location")  //
		("output,o", po::value<string>(&outputFile)->required(), "file where to write output");
	rerunBoostPO(parsed, license_desc, vm, argv, "license issue", global);
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->loadPrivateKey_file(private_key_file);
	string signedData(crypto->signString(data));
	if (outputFile != "cout") {
		ofstream ofile;
		ofile.open(outputFile, ios::trunc);
		if (!ofile.is_open()) {
			throw logic_error("can't create [" + outputFile + "]");
		}
		ofile << signedData;
		ofile.close();
	} else {
		cout << signedData << endl;
	}
}

int CommandLineParser::parseCommandLine(int argc, const char **argv) {
	if (argc == 1) {
		printBasicHelp(argv[0]);
		return 1;
	}
	int result = 0;
	po::options_description global("Global options");
	global.add_options()("verbose,v", "Turn on verbose output");
	po::options_description hidden("Hidden options");
	hidden.add_options()("command", po::value<std::vector<std::string>>(),
						 "command to execute: project init, project list, license list, license issue")(
		"subargs", po::value<std::vector<std::string>>(), "Arguments for command, use option --help to see");

	po::positional_options_description pos;
	pos.add("command", 2).add("subargs", -1);

	po::variables_map vm;

	po::parsed_options parsed =
		po::command_line_parser(argc, argv).options(global).options(hidden).positional(pos).allow_unregistered().run();
	po::store(parsed, vm);
	std::vector<std::string> cmds = vm["command"].as<std::vector<std::string>>();
	if (cmds.size() == 0 || cmds.size() == 1) {
		printBasicHelp(argv[0]);
		return 1;
	}
	bool verbose = vm.count("verbose") > 0;
	try {
		if (cmds[0] == "project") {
			if (cmds[1].substr(0, 4) == "init") {
				initializeProject(parsed, vm, argv, global);
			} else if (cmds[1] == "list") {
				po::options_description project_desc("project " + cmds[1] + " options");
				boost::optional<string> project_folder;
				project_desc.add_options()  //
					("projects-folder,p", po::value<boost::optional<string>>(&project_folder),
					 "path to where project configurations are stored.")  //
					("help", "Print this help.");  //

			} else {
				std::cerr << endl << "command " << cmds[0] << " " << cmds[1] << " not recognized.";
				printBasicHelp(argv[0]);
				result = 1;
			}
		} else if (cmds[0] == "license") {
			if (cmds[1] == "issue") {
				issueLicense(parsed, vm, argv, global);
			} else {
				printBasicHelp(argv[0]);
				result = 1;
			}
		} else if (cmds[0] == "test") {
			po::options_description license_desc("test " + cmds[1] + " options");
			if (cmds[1] == "sign") {
				test_sign(parsed, vm, argv, global);
			} else {
				result = 1;
			}
		} else {
			printBasicHelp(argv[0]);
			result = 1;
		}
	} catch (const invalid_argument &e) {
		printBasicHelp(argv[0]);
		cout << endl << "Parameter error: " << e.what() << endl;
		result = 1;
	}
	return result;
}

} /* namespace license */
