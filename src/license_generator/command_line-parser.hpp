/*
 * command_line-parser.hpp
 *
 *  Created on: Oct 20, 2019
 *      Author: Gabriele Contini
 */

#ifndef SRC_TOOLS_LICENSE_GENERATOR_COMMAND_LINE_PARSER_HPP_
#define SRC_TOOLS_LICENSE_GENERATOR_COMMAND_LINE_PARSER_HPP_

#include <string>
#include <boost/optional.hpp>

namespace license {

class CommandLineParser {
private:
	const bool m_verbose;
	int project_init(
			const std::string& project_name, //
			const boost::optional<std::string>& primary_key, //
			const boost::optional<std::string>& public_key, //
			const boost::optional<std::string>& project_folder, //
			const boost::optional<std::string>& source_folder);
	CommandLineParser(bool verbose);
	virtual ~CommandLineParser();

public:
	/**
	 * Parse the `olm` command line.
	 * Available commands:
	 * <ul>
	 * <li>project init</li>
	 * <li>project list</li>
	 *
	 * <ul>
	 * @param argc
	 * @param argv
	 * @return
	 */
	static int parseCommandLine(int argc, const char** argv);
};

} /* namespace license */

#endif /* SRC_TOOLS_LICENSE_GENERATOR_COMMAND_LINE_PARSER_HPP_ */
