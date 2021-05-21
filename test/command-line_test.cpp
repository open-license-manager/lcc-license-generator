#define BOOST_TEST_MODULE test_command_line

#include <string>
#include <fstream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>
#if (BOOST_VERSION > 107000)
#include <boost/test/tools/output_test_stream.hpp>
#else
#include <boost/test/output_test_stream.hpp>
#endif
#include <iostream>

#include <build_properties.h>
#include "../src/license_generator/command_line-parser.hpp"
#include "../src/ini/SimpleIni.h"
#include "../src/base_lib/base.h"
#include "cout_redirect.hpp"

namespace fs = boost::filesystem;
using namespace license;
using namespace std;

namespace license {
namespace test {

static void create_project(const fs::path& projects_folder, const fs::path& expectedPrivateKey,
						   const fs::path& expected_public_key, const fs::path& mock_source_folder,
						   const string& project_name) {
	fs::remove_all(projects_folder);
	BOOST_CHECK_MESSAGE(!fs::exists(expectedPrivateKey),
						"Private key " + expectedPrivateKey.string() + " can't be deleted.");
	BOOST_CHECK_MESSAGE(!fs::exists(expected_public_key),
						"Public key " + expected_public_key.string() + " can't be deleted.");
	const string mock_source = mock_source_folder.string();
	const string projects_str = projects_folder.string();
	int argc = 9;
	const char* argv1[] = {"lcc",
						   "project",
						   "init",
						   "-n",
						   project_name.c_str(),
						   "--projects-folder",
						   projects_str.c_str(),
						   "--templates",
						   mock_source.c_str()};
	// initialize_project
	int result = CommandLineParser::parseCommandLine(argc, argv1);
	BOOST_CHECK_EQUAL(result, 0);
	BOOST_REQUIRE_MESSAGE(fs::exists(expectedPrivateKey), "Private key " + expectedPrivateKey.string() + " created.");
	BOOST_CHECK_MESSAGE(fs::exists(expected_public_key), "Public key " + expected_public_key.string() + " created.");
}

BOOST_AUTO_TEST_CASE(product_initialize_issue_license) {
	const string project_name("TEST");
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path projects_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "lcc_projects");
	const fs::path expected_project_folder(projects_folder / project_name);
	const fs::path expectedPrivateKey(projects_folder / project_name / PRIVATE_KEY_FNAME);
	const fs::path expected_public_key(projects_folder / project_name / "include" / "licensecc" / project_name /
									   PUBLIC_KEY_INC_FNAME);

	create_project(projects_folder, expectedPrivateKey, expected_public_key, mock_source_folder, project_name);
	const string private_key_str = expectedPrivateKey.string();
	const string project_folder_str = expected_project_folder.string();
	// issue license in standard location
	int argc = 9;
	const char* argv2[] = {"lcc",
						   "license",
						   "issue",
						   "--" PARAM_PRIMARY_KEY,
						   private_key_str.c_str(),
						   "--" PARAM_LICENSE_OUTPUT,
						   "my_license.lic",
						   "--" PARAM_PROJECT_FOLDER,
						   project_folder_str.c_str()};
	int result = CommandLineParser::parseCommandLine(argc, argv2);
	BOOST_CHECK_EQUAL(result, 0);
	fs::path expected_license("my_license.lic");
	BOOST_REQUIRE_MESSAGE(fs::exists(expected_license), "License " + expected_license.string() + " created.");
	// load a license, check the project name corresponds and there are no extra elements.
	CSimpleIniA ini;
	ini.LoadFile(expected_license.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize(project_name.c_str()) == 2, "Section [" + project_name + "] has 2 elements");
}

#if BOOST_VERSION > 106500
BOOST_AUTO_TEST_CASE(product_initialize_issue_license_multi_feature) {
	const string project_name("TEST");
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path projects_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "lcc_projects");
	const fs::path expected_project_folder(projects_folder / project_name);
	const fs::path expectedPrivateKey(projects_folder / project_name / PRIVATE_KEY_FNAME);
	const fs::path expected_public_key(projects_folder / project_name / "include" / "licensecc" / project_name /
									   PUBLIC_KEY_INC_FNAME);

	create_project(projects_folder, expectedPrivateKey, expected_public_key, mock_source_folder, project_name);
	const string private_key_str = expectedPrivateKey.string();
	const string project_folder_str = expected_project_folder.string();
	// issue license in standard location
	int argc = 11;
	const char* argv2[] = {"lcc",
						   "license",
						   "issue",
						   "--" PARAM_PRIMARY_KEY,
						   private_key_str.c_str(),
						   "--" PARAM_LICENSE_OUTPUT,
						   "my_license_multi.lic",
						   "--" PARAM_PROJECT_FOLDER,
						   project_folder_str.c_str(),
						   "-f",
						   "TEST,feature1"};
	int result = CommandLineParser::parseCommandLine(argc, argv2);
	BOOST_CHECK_EQUAL(result, 0);
	fs::path expected_license("my_license_multi.lic");
	BOOST_REQUIRE_MESSAGE(fs::exists(expected_license), "License " + expected_license.string() + " created.");
	// load a license, check the project name corresponds and there are no extra elements.
	CSimpleIniA ini;
	ini.LoadFile(expected_license.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize(project_name.c_str()) == 2, "Section [" + project_name + "] has 2 elements");
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("feature1") == 2, "Section [feature1] has 2 elements");
}
#endif

BOOST_AUTO_TEST_CASE(issue_license_help) {
	int argc = 4;
	const char* argv1[] = {"lcc", "license", "issue", "-h"};
	// initialize_project
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		int result = CommandLineParser::parseCommandLine(argc, argv1);
	}
	string stdout_str = output.str();
	BOOST_CHECK_MESSAGE(stdout_str.find(PARAM_CLIENT_SIGNATURE) != string::npos,
						"command help was print out " + stdout_str);
}

/**
 * The project name should not contain '\ / [ ]' charactoers
 */
BOOST_AUTO_TEST_CASE(init_project_name_wrong) {
	const string project_name("a/TEST");
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path projects_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "lcc_projects_wa");
	const string mock_source = mock_source_folder.string();
	const string projects_str = projects_folder.string();

	int argc = 9;
	const char* argv1[] = {"lcc",
						   "project",
						   "init",
						   "-n",
						   project_name.c_str(),
						   "--projects-folder",
						   projects_str.c_str(),
						   "--templates",
						   mock_source.c_str()};
	int result;
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		result = CommandLineParser::parseCommandLine(argc, argv1);
	}
	string stdout_str = output.str();
	BOOST_CHECK_EQUAL(result, 1);
	BOOST_CHECK_MESSAGE(stdout_str.find("rror") != string::npos && stdout_str.find("project name") != string::npos,
						"error was print out " + stdout_str);
}

}  // namespace test
}  // namespace license
