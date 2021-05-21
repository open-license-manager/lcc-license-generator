#define BOOST_TEST_MODULE test_project

#include <fstream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <build_properties.h>

#include "../src/license_generator/project.hpp"
#include "../src/ini/SimpleIni.h"
#include "../src/base_lib/base.h"

namespace fs = boost::filesystem;
using namespace license;
using namespace std;

BOOST_AUTO_TEST_CASE(project_initialize) {
	const string project_name("TEST");
	const fs::path mock_source_folder(fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path project_folder(fs::path(PROJECT_TEST_TEMP_DIR) / "product_initialize");
	const fs::path expectedPrivateKey(project_folder / project_name / PRIVATE_KEY_FNAME);
	const fs::path expected_public_key(project_folder / project_name / "include" / "licensecc" / project_name /
									   PUBLIC_KEY_INC_FNAME);

	fs::remove_all(project_folder);
	BOOST_CHECK_MESSAGE(!fs::exists(expectedPrivateKey),
						"Private key " + expectedPrivateKey.string() + " can't be deleted.");
	BOOST_CHECK_MESSAGE(!fs::exists(expected_public_key),
						"Public key " + expected_public_key.string() + " can't be deleted.");

	Project prj(project_name, project_folder.string(), mock_source_folder.string(), false);
	prj.initialize();

	BOOST_CHECK_MESSAGE(fs::exists(expectedPrivateKey), "Private key " + expectedPrivateKey.string() + " created.");
	BOOST_REQUIRE_MESSAGE(fs::exists(expected_public_key), "Public key " + expected_public_key.string() + " created.");

	// read the public key file
	std::ifstream t(expected_public_key.string());
	std::string pub_key((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	BOOST_CHECK_MESSAGE(pub_key.find("TEST") != std::string::npos, "Project defined");
}

BOOST_AUTO_TEST_CASE(project_initialize_force) {}
