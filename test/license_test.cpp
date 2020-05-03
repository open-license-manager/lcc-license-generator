#define BOOST_TEST_MODULE test_license

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/version.hpp>
#if (BOOST_VERSION > 107000)
#include <boost/test/tools/output_test_stream.hpp>
#else
#include <boost/test/output_test_stream.hpp>
#endif
#include <iostream>
#include <build_properties.h>

#include "../src/base_lib/base.h"
#include "../src/ini/SimpleIni.h"
#include "../src/license_generator/license.hpp"
#include "cout_redirect.hpp"

namespace license {
namespace test {
namespace fs = boost::filesystem;
using namespace license;
using namespace std;

struct MyGlobalFixture {
	MyGlobalFixture() {}

	void setup() {
		BOOST_TEST_MESSAGE("setup temp project ");
		if (fs::exists(project_path)) {
			fs::remove_all(project_path);
		}
		bool ok = fs::create_directories(licenses_path);
		BOOST_REQUIRE_MESSAGE(ok, string("Error creating ") + licenses_path.string());
		fs::path pkf = fs::path(PROJECT_TEST_SRC_DIR) / "data" / PRIVATE_KEY_FNAME;
		fs::copy_file(pkf, project_path / PRIVATE_KEY_FNAME);
	}

	void teardown() {
		/*if (fs::exists(project_path)) {
			fs::remove_all(project_path);
		}*/
	}

	~MyGlobalFixture(){};
	const static fs::path project_path;
	const static fs::path licenses_path;
	const static string licenses_path_str;
};

const fs::path MyGlobalFixture::project_path(fs::path(fs::path(PROJECT_TEST_TEMP_DIR) / "test_project"));
const fs::path MyGlobalFixture::licenses_path(project_path / "licenses");
const std::string MyGlobalFixture::licenses_path_str(licenses_path.string());

// this test is incompatible with older version of boost
#ifdef BOOST_TEST_GLOBAL_FIXTURE

BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

/**
 * Test date normalization
 */
BOOST_AUTO_TEST_CASE(license_structure) {
	const fs::path licLocation = MyGlobalFixture::licenses_path / "test.lic";
	const string lic_location_str = licLocation.string();
	License license(&lic_location_str, MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_EXPIRY_DATE, "19290111");
	license.write_license();

	BOOST_REQUIRE_MESSAGE(fs::exists(licLocation), "license has been created");
	CSimpleIniA ini;
	ini.LoadFile(licLocation.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("TEST_PROJECT") == 3, "Section TEST_PROJECT has 3 elements");
	BOOST_CHECK_MESSAGE(string(ini.GetValue("TEST_PROJECT", PARAM_EXPIRY_DATE, "X")) == "1929-01-11",
						"Section TEST_PROJECT has expiry date");
	// std::cout << ini.GetValue("TEST_PROJECT", PARAM_EXPIRY_DATE, "X") << endl;
}

BOOST_AUTO_TEST_CASE(generate_license_subdir) {
	const fs::path licLocation = MyGlobalFixture::licenses_path / "test_folder" / "test.lic";
	const string lic_location_str = licLocation.string();
	License license(&lic_location_str, MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_EXPIRY_DATE, "1929-11-11");
	license.write_license();

	BOOST_CHECK_MESSAGE(fs::exists(licLocation), "license has been created");
}

BOOST_AUTO_TEST_CASE(generate_license_with_relative_path) {
	const fs::path license_rel_path = fs::path("license.lic");
	const string license_rel_path_str = license_rel_path.string();
	License license(&license_rel_path_str, MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_FEATURE_NAMES, "my_fantastic_softwAre");
	license.write_license();
	BOOST_REQUIRE_MESSAGE(fs::exists(license_rel_path), "license has been created");
}

BOOST_AUTO_TEST_CASE(license_stdout) {
	boost::test_tools::output_test_stream output;
	{
		cout_redirect guard(output.rdbuf());

		License license(nullptr, MyGlobalFixture::project_path.string());
		license.add_parameter(PARAM_FEATURE_NAMES, "my_fantastic_softwAre");
		license.write_license();
	}
	string stdout_str = output.str();
	BOOST_CHECK_MESSAGE(stdout_str.find("[MY_FANTASTIC_SOFTWARE]") != string::npos,
						"license has been written to stdout " + stdout_str);
}

BOOST_AUTO_TEST_CASE(generate_license_features) {
	const fs::path licFile = MyGlobalFixture::licenses_path / "myclient2.lic";
	const string lic_location_str = licFile.string();
	License license(&lic_location_str, MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_FEATURE_NAMES, "my_fantastic_softwAre,another_feature");
	license.write_license();
	BOOST_REQUIRE_MESSAGE(fs::exists(licFile), "license has been created");
	CSimpleIniA ini;
	ini.LoadFile(licFile.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("MY_FANTASTIC_SOFTWARE") == 2,
						"Section [MY_FANTASTIC_SOFTWARE] has 2 elements");
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("ANOTHER_FEATURE") == 2, "Section [ANOTHER_FEATURE] has 2 elements");
}

BOOST_AUTO_TEST_CASE(extend_license) {
	const fs::path licFile = MyGlobalFixture::licenses_path / "myclient.lic";
	const string lic_location_str = licFile.string();
	License license(&lic_location_str, MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_EXPIRY_DATE, "1929-11-11");
	license.add_parameter(PARAM_CLIENT_SIGNATURE, "XXX-XXX-XXX");
	license.write_license();
	BOOST_REQUIRE_MESSAGE(fs::exists(licFile), "license has been created");
	CSimpleIniA ini;
	ini.LoadFile(licFile.c_str());
	BOOST_CHECK_MESSAGE(string(ini.GetValue("TEST_PROJECT", PARAM_EXPIRY_DATE)) == "1929-11-11", "Date was written");

	License license_renew(&lic_location_str, MyGlobalFixture::project_path.string());
	const string new_date("2020-05-01");
	license_renew.add_parameter(PARAM_EXPIRY_DATE, new_date.c_str());
	license_renew.write_license();
	ini.Reset();
	ini.LoadFile(licFile.c_str());
	BOOST_CHECK_MESSAGE(ini.GetValue("TEST_PROJECT", PARAM_EXPIRY_DATE) == new_date, "license extended");
	BOOST_CHECK_MESSAGE(ini.GetValue("TEST_PROJECT", PARAM_CLIENT_SIGNATURE) == string("XXX-XXX-XXX"),
						"license extended");
}

#else
BOOST_AUTO_TEST_CASE(mock) { BOOST_CHECKPOINT("Mock test for older boost versions"); }
#endif
}  // namespace test
}  // namespace license
