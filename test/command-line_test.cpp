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
						   const string& project_name, int key_bits = DEFAULT_RSA_KEY_BITS) {
	fs::remove_all(projects_folder);
	BOOST_CHECK_MESSAGE(!fs::exists(expectedPrivateKey),
						"Private key " + expectedPrivateKey.string() + " can't be deleted.");
	BOOST_CHECK_MESSAGE(!fs::exists(expected_public_key),
						"Public key " + expected_public_key.string() + " can't be deleted.");
	const char* argv1[] = {"lcc",
						   "project",
						   "init",
						   "-n",
						   project_name.c_str(),
						   "--projects-folder",
						   projects_folder.c_str(),
						   "--templates",
						   mock_source_folder.c_str(),
						   "-k",
						   to_string(key_bits).c_str()};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	// initialize_project
	int result = CommandLineParser::parseCommandLine(argc, argv1);
	BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	BOOST_REQUIRE_MESSAGE(fs::exists(expectedPrivateKey), "Private key " + expectedPrivateKey.string() + " created.");
	BOOST_CHECK_MESSAGE(fs::exists(expected_public_key), "Public key " + expected_public_key.string() + " created.");
}

static fs::path create_project(const string& project_name, int key_bits = DEFAULT_RSA_KEY_BITS) {
	// check here! it is creating files in the source tree?
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path projects_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "lcc_projects");
	const fs::path expected_project_folder(projects_folder / project_name);
	const fs::path expectedPrivateKey(expected_project_folder / PRIVATE_KEY_FNAME);
	const fs::path expected_public_key(expected_project_folder / "include" / "licensecc" / project_name /
									   PUBLIC_KEY_INC_FNAME);

	create_project(projects_folder, expectedPrivateKey, expected_public_key, mock_source_folder, project_name,
				   key_bits);

	return expected_project_folder;
}

BOOST_AUTO_TEST_CASE(product_initialize_issue_license) {
	const string project_name("TEST");
	const fs::path expected_project_folder = create_project(project_name);
	// issue license in standard location
	const fs::path private_key_path = expected_project_folder / PRIVATE_KEY_FNAME;
	const char* argv2[] = {"lcc",
						   "license",
						   "issue",
						   "--" PARAM_PRIMARY_KEY,
						   private_key_path.c_str(),
						   "--" PARAM_LICENSE_OUTPUT,
						   "my_license.lic",
						   "--" PARAM_PROJECT_FOLDER,
						   expected_project_folder.c_str()};
	int argc = sizeof(argv2) / sizeof(argv2[0]);
	int result = CommandLineParser::parseCommandLine(argc, argv2);
	BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	fs::path expected_license("my_license.lic");
	BOOST_REQUIRE_MESSAGE(fs::exists(expected_license), "License " + expected_license.string() + " created.");
	// load a license, check the project name corresponds and there are no extra elements.
	CSimpleIniA ini;
	ini.LoadFile(expected_license.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize(project_name.c_str()) == 2, "Section [" + project_name + "] has 2 elements");
	ini.GetLongValue(project_name.c_str(), "lic_ver", LICENSE_VERSION_210);	 // new license version for 2048 keys
}

#if BOOST_VERSION > 106500
BOOST_AUTO_TEST_CASE(product_initialize_issue_license_multi_feature) {
	const string project_name("TEST");
	const fs::path project_folder = create_project(project_name);
	const fs::path expectedPrivateKey(project_folder / PRIVATE_KEY_FNAME);
	const fs::path expected_public_key(project_folder / "include" / "licensecc" / project_name / PUBLIC_KEY_INC_FNAME);

	// issue license in standard location
	const fs::path private_key_path = project_folder / PRIVATE_KEY_FNAME;
	const char* argv2[] = {"lcc",
						   "license",
						   "issue",
						   "--" PARAM_PRIMARY_KEY,
						   private_key_path.c_str(),
						   "--" PARAM_LICENSE_OUTPUT,
						   "my_license_multi.lic",
						   "--" PARAM_PROJECT_FOLDER,
						   project_folder.c_str(),
						   "-f",
						   "TEST,feature1"};
	int argc = sizeof(argv2) / sizeof(argv2[0]);
	int result = CommandLineParser::parseCommandLine(argc, argv2);
	BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	fs::path expected_license("my_license_multi.lic");
	BOOST_REQUIRE_MESSAGE(fs::exists(expected_license), "License " + expected_license.string() + " created.");
	// load a license, check the project name corresponds and there are no extra elements.
	CSimpleIniA ini;
	ini.LoadFile(expected_license.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize(project_name.c_str()) == 2, "Section [" + project_name + "] has 2 elements");
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("feature1") == 2, "Section [feature1] has 2 elements");
}

BOOST_AUTO_TEST_CASE(product_initialize_1024_bit_issue_license) {
	const string project_name("TEST1024");
	const fs::path project_folder = create_project(project_name, 1024);
	const fs::path expectedPrivateKey(project_folder / PRIVATE_KEY_FNAME);
	const fs::path expected_public_key(project_folder / "include" / "licensecc" / project_name / PUBLIC_KEY_INC_FNAME);

	// issue license in standard location
	const fs::path private_key_path = project_folder / PRIVATE_KEY_FNAME;
	const char* argv2[] = {"lcc",
						   "license",
						   "issue",
						   "--" PARAM_PRIMARY_KEY,
						   private_key_path.c_str(),
						   "--" PARAM_LICENSE_OUTPUT,
						   "my_license_1024bit.lic",
						   "--" PARAM_PROJECT_FOLDER,
						   project_folder.c_str()};
	int argc = sizeof(argv2) / sizeof(argv2[0]);
	int result = CommandLineParser::parseCommandLine(argc, argv2);
	BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	fs::path expected_license("my_license_1024bit.lic");
	BOOST_REQUIRE_MESSAGE(fs::exists(expected_license), "License " + expected_license.string() + " created.");
	// load a license, check the project name corresponds and there are no extra elements.
	CSimpleIniA ini;
	ini.LoadFile(expected_license.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize(project_name.c_str()) == 2, "Section [" + project_name + "] has 2 elements");
	ini.GetLongValue(project_name.c_str(), "lic_ver", LICENSE_VERSION_200);	 // old license version for 1024 keys
}
#endif

BOOST_AUTO_TEST_CASE(issue_license_help) {
	const char* argv1[] = {"lcc", "license", "issue", "-h"};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
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
 * Test custom-value command line option
 */
BOOST_AUTO_TEST_CASE(issue_license_custom_value) {
	const string project_name("TEST1024");
	const fs::path project_folder = create_project(project_name, 1024);
	const fs::path licLocation = fs::path(PROJECT_TEST_TEMP_DIR) / "test_custom_2.lic";

	// Test with custom-value parameters
	const char* argv1[] = {"lcc",
						   "license",
						   "issue",
						   "--custom-value",
						   "custom_key1=custom_value1",
						   "--custom-value",
						   "custom_key2=custom_value2",
						   "--custom-value",
						   "another_custom=some_value",
						   "--output-file-name",
						   licLocation.c_str(),
						   "--project-folder",
						   project_folder.c_str()};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	int result = CommandLineParser::parseCommandLine(argc, argv1);
	BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	BOOST_REQUIRE_MESSAGE(fs::exists(licLocation), "license has been created");
	// Check that custom parameters are present in the license
	CSimpleIniA ini;
	ini.LoadFile(licLocation.c_str());

	// Check that custom parameters are present in the license
	BOOST_CHECK_MESSAGE(string(ini.GetValue(project_name.c_str(), "custom_key1", "X")) == "custom_value1",
						"Custom key1 has correct value in license");
	BOOST_CHECK_MESSAGE(string(ini.GetValue(project_name.c_str(), "custom_key2", "X")) == "custom_value2",
						"Custom key2 has correct value in license");
	BOOST_CHECK_MESSAGE(string(ini.GetValue(project_name.c_str(), "another_custom", "X")) == "some_value",
						"Another custom key has correct value in license");
}

/**
 * Test invalid custom-value format
 */
BOOST_AUTO_TEST_CASE(issue_license_invalid_custom_value) {
	// Test with invalid custom-value format (missing =)
	const char* argv1[] = {"lcc",
						   "license",
						   "issue",
						   "--custom-value",
						   "invalid_format_no_equals",
						   "--output-file-name",
						   "test_invalid.lic"};
	int argc = sizeof(argv1) / sizeof(argv1[0]);

	boost::test_tools::output_test_stream output;
	int result = CommandLineParser::parseCommandLine(argc, argv1);

	BOOST_CHECK_MESSAGE(result == FUNC_RET_ERROR, "The function must return an error value");
}

/**
 * The project name should not contain '\ / [ ]' charactoers
 */
BOOST_AUTO_TEST_CASE(init_project_name_wrong) {
	const string project_name("a/TEST");
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path projects_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "lcc_projects_wa");

	const char* argv1[] = {"lcc",
						   "project",
						   "init",
						   "-n",
						   project_name.c_str(),
						   "--projects-folder",
						   projects_folder.c_str(),
						   "--templates",
						   mock_source_folder.c_str()};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	int result;
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		result = CommandLineParser::parseCommandLine(argc, argv1);
	}
	string stdout_str = output.str();
	BOOST_CHECK_EQUAL(result, FUNC_RET_ERROR);
	BOOST_CHECK_MESSAGE(stdout_str.find("rror") != string::npos && stdout_str.find("project name") != string::npos,
						"error was print out " + stdout_str);
}

/**
 * Test wrong key size for project init. Valid values are 1024, 2048, or 4096.
 */
BOOST_AUTO_TEST_CASE(init_project_key_size_wrong) {
	const string project_name("TEST");
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path projects_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "lcc_projects_wa");

	const char* argv1[] = {"lcc",
						   "project",
						   "init",
						   "-n",
						   project_name.c_str(),
						   "--projects-folder",
						   projects_folder.c_str(),
						   "--templates",
						   mock_source_folder.c_str(),
						   "--key-bits",
						   "42"};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	int result;
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		result = CommandLineParser::parseCommandLine(argc, argv1);
	}
	string stdout_str = output.str();
	BOOST_CHECK_EQUAL(result, FUNC_RET_ERROR);
	BOOST_CHECK_MESSAGE(stdout_str.find("rror") != string::npos && stdout_str.find("key") != string::npos,
						"error was print out " + stdout_str);
}

BOOST_AUTO_TEST_CASE(version_command_default) {
	const char* argv1[] = {"lcc", "version"};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		int result = CommandLineParser::parseCommandLine(argc, argv1);
		BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	}
	string stdout_str = output.str();
	BOOST_CHECK_MESSAGE(stdout_str.find(PROJECT_VERSION) != string::npos,
						"Default version command should output PROJECT_VERSION: " + stdout_str);
}

BOOST_AUTO_TEST_CASE(version_command_numeric) {
	const char* argv1[] = {"lcc", "version", "--numeric"};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		int result = CommandLineParser::parseCommandLine(argc, argv1);
		BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	}
	string stdout_str = output.str();
	// Convert PROJECT_INT_VERSION to string for comparison
	string expected_version = std::to_string(PROJECT_INT_VERSION);
	BOOST_CHECK_MESSAGE(stdout_str.find(expected_version) != string::npos,
						"Numeric version command should output PROJECT_INT_VERSION: " + stdout_str);
}

BOOST_AUTO_TEST_CASE(version_command_help) {
	const char* argv1[] = {"lcc", "version", "--help"};
	int argc = sizeof(argv1) / sizeof(argv1[0]);
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());
		int result = CommandLineParser::parseCommandLine(argc, argv1);
		BOOST_CHECK_EQUAL(result, FUNC_RET_OK);
	}
	string stdout_str = output.str();
	BOOST_CHECK_MESSAGE(stdout_str.find("numeric") != string::npos,
						"Version help should mention the numeric option: " + stdout_str);
}
}  // namespace test
}  // namespace license
