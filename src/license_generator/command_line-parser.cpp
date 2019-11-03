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
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <build_properties.h>

#include "../base_lib/base64.h"
#include "command_line-parser.hpp"

#include "project.hpp"

namespace po = boost::program_options;
using namespace std;

namespace license {

static void printHelpHeader(const char *prog_name) {
	cout << endl;
	cout << prog_name << " Version " << PROJECT_VERSION << ". Usage:" << endl;
}

static void printHelp(const char *prog_name,
		const po::options_description &options) {
	printHelpHeader(prog_name);
	cout << prog_name << " [options] product_name1 product_name2 ... " << endl
			<< endl;
	cout
			<< " product_name1 ... = Product name. This string must match the one passed by the software."
			<< endl;
	cout << options << endl;
}

static void printBasicHelp(const char *prog_name) {
	printHelpHeader(prog_name);
	cout << prog_name << " [command] [options]" << endl;
	cout
			<< " available commands: \"product initialize\", \"product list\", \"license issue\", \"license list\""
			<< endl;
	cout << " to see specific command options type: " << prog_name
			<< " [command] --help" << endl << endl;

}

CommandLineParser::CommandLineParser(bool verb) :
		m_verbose(verb) {

}

CommandLineParser::~CommandLineParser() {

}

int CommandLineParser::project_init(const std::string &project_name, //
		const boost::optional<string> &primary_key, //
		const boost::optional<string> &public_key, //
		const boost::optional<string> &project_folder, //
		const boost::optional<string> &source_folder) {
	cout << source_folder.is_initialized() << endl;
	Project project(project_name,
			project_folder.is_initialized() ? project_folder.get() : "",
			source_folder.is_initialized() ? source_folder.get() : "");

	return project.initialize();
}

static void rerunBoostPO(const po::parsed_options &parsed,
		const po::options_description &project_desc, po::variables_map vm,
		const char **argv, const std::vector<std::string> &cmds,
		const po::options_description &global) {
	// Collect all the unrecognized options from the first pass. This will include the
	// (positional) command name, so we need to erase that.
	// Parse again...
	std::vector<std::string> opts = po::collect_unrecognized(parsed.options,
			po::include_positional);
	opts.erase(opts.begin());
	po::store(po::command_line_parser(opts).options(project_desc).run(), vm);
	try {
		po::notify(vm);
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << endl;
		printHelpHeader(argv[0]);
		cout << argv[0] << " product " << cmds[1] << " [options]" << endl;
		global.print(cout);
		project_desc.print(cout);
	}
}

int CommandLineParser::parseCommandLine(int argc, const char **argv) {
	if (argc == 1) {
		printBasicHelp(argv[0]);
		return 1;
	}
	po::options_description global("Global options");
	global.add_options()("verbose,v", "Turn on verbose output");
	po::options_description hidden("Hidden options");
	hidden.add_options()("command", po::value<std::vector<std::string>>(),
			"command to execute: product init, product list, license list, license issue")(
			"subargs", po::value<std::vector<std::string>>(),
			"Arguments for command, use option --help to see");

	po::positional_options_description pos;
	pos.add("command", 2).add("subargs", -1);

	po::variables_map vm;

	po::parsed_options parsed = po::command_line_parser(argc, argv).options(
			global).options(hidden).positional(pos).allow_unregistered().run();
	po::store(parsed, vm);
	std::vector<std::string> cmds =
			vm["command"].as<std::vector<std::string>>();
	if (cmds.size() == 0 || cmds.size() == 1) {
		printBasicHelp(argv[0]);
		return 1;
	}
	bool verbose = vm.count("verbose") > 0;
	CommandLineParser cmd_parser(verbose);

	if (cmds[0] == "project") {
		po::options_description project_desc("product " + cmds[1] + " options");
		if (cmds[1].substr(0, 4) == "init") {
			string project_name;
			boost::optional<string> primary_key;
			boost::optional<string> public_key;
			boost::optional<string> project_folder;
			boost::optional<string> source_folder;
			project_desc.add_options() //
			("name,n", po::value<std::string>(&project_name)->required(),
					"New project name (required).") //
			("primary-key", po::value<boost::optional<string>>(&primary_key),
					"use externally generated primary key, public key must also be specified.") //
			("public-key", po::value<boost::optional<string>>(&public_key),
					"Use externally generated public key, private key must also be specified.") //
			("products-folder,p",
					po::value<boost::optional<string>>(&project_folder),
					"path to where product configurations are stored.") //
			("source,s", po::value<boost::optional<string>>(&source_folder),
					"path to the library source.") //
			("help", "Print this help."); //

			rerunBoostPO(parsed, project_desc, vm, argv, cmds, global);
			cmd_parser.project_init(project_name, primary_key, public_key,
					project_folder, source_folder);
		} else if (cmds[1] == "list") {
			boost::optional<string> project_folder;
			project_desc.add_options() //
					("products-folder,p",
							po::value<boost::optional<string>>(&project_folder),
							"path to where product configurations are stored.") //
			("help", "Print this help."); //

		} else {
			std::cerr << endl << "command " << cmds[0] << " " << cmds[1]
					<< " not recognized.";
			printBasicHelp(argv[0]);
		}
	}
	/*po::options_description visibleOptions = configureProgramOptions();
	 //positional options must be added to standard options
	 po::options_description allOptions;
	 allOptions.add(visibleOptions).add_options()("product",
	 po::value<vector<string>>(), "product names");

	 po::positional_options_description p;
	 p.add("product", -1);

	 po::variables_map vm;
	 po::store(
	 po::command_line_parser(argc, argv).options(allOptions).positional(
	 p).run(), vm);
	 po::notify(vm);
	 if (vm.count("help") || argc == 1) {
	 printHelp(argv[0], visibleOptions);
	 return 0;
	 }*/

	/*if (vm.count("output")) {
	 const std::string fname = vm["output"].as<string>();

	 fstream ofstream(fname, std::ios::out | std::ios::app);
	 if (!ofstream.is_open()) {
	 cerr << "can't open file [" << fname << "] for output." << endl
	 << " error: " << strerror(errno) << endl;
	 exit(3);
	 }
	 if (vm.count("base64")) {
	 generateB64Licenses(vm, ofstream);
	 } else {
	 generateAndOutputLicenses(vm, ofstream);
	 }
	 ofstream.close();
	 } else {
	 if (vm.count("base64")) {
	 generateB64Licenses(vm, cout);
	 } else {
	 generateAndOutputLicenses(vm, cout);
	 }
	 }*/
	return 0;
}

} /* namespace license */
