#define BOOST_TEST_MODULE test_license

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <build_properties.h>

#include "../src/base_lib/base.h"
#include "../src/ini/SimpleIni.h"
#include "../src/license_generator/license.hpp"

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
	static fs::path project_path;
	static fs::path licenses_path;
};

fs::path MyGlobalFixture::project_path(fs::path(fs::path(PROJECT_TEST_TEMP_DIR) / "test_project"));
fs::path MyGlobalFixture::licenses_path(project_path / "licenses");

// this test is incompatible with older version of boost
#ifdef BOOST_TEST_GLOBAL_FIXTURE

BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

BOOST_AUTO_TEST_CASE(license_structure) {
	License license("test", MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_EXPIRY_DATE, "19290111");
	license.write_license();

	fs::path licFile = MyGlobalFixture::licenses_path / "test.lic";
	BOOST_REQUIRE_MESSAGE(fs::exists(licFile), "license has been created");
	CSimpleIniA ini;
	ini.LoadFile(licFile.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("TEST_PROJECT") == 3, "Section TEST_PROJECT has 3 elements");
	BOOST_CHECK_MESSAGE(ini.GetValue("TEST_PROJECT", PARAM_EXPIRY_DATE, "X") != "1929-01-11",
						"Section TEST_PROJECT has expiry date");
}

BOOST_AUTO_TEST_CASE(generate_license_subdir) {
	License license("test_folder/test", MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_EXPIRY_DATE, "1929-11-11");
	license.write_license();

	BOOST_CHECK_MESSAGE(fs::exists(MyGlobalFixture::licenses_path / "test_folder" / "test.lic"),
						"license has been created");
}

BOOST_AUTO_TEST_CASE(generate_license_with_absolute_path) {
	const fs::path license_abs_path = fs::path(PROJECT_TEST_TEMP_DIR) / "test_abs_path.lic";
	License license(license_abs_path.string(), MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_PRODUCT_NAME, "my_fantastic_softwAre");
	license.write_license();

	BOOST_CHECK_MESSAGE(license.get_license_file_path() == license_abs_path.string(),
						"file created " + license.get_license_file_path());
	BOOST_REQUIRE_MESSAGE(fs::exists(license_abs_path), "license has been created");
}

BOOST_AUTO_TEST_CASE(generate_license_product) {
	License license("myclient", MyGlobalFixture::project_path.string());
	license.add_parameter(PARAM_PRODUCT_NAME, "my_fantastic_softwAre");
	license.write_license();

	fs::path licFile = MyGlobalFixture::licenses_path / "myclient.lic";
	BOOST_CHECK_MESSAGE(license.get_license_file_path() == licFile.string(),
						"file created " + license.get_license_file_path());
	BOOST_REQUIRE_MESSAGE(fs::exists(licFile), "license has been created");
	CSimpleIniA ini;
	ini.LoadFile(licFile.c_str());
	BOOST_CHECK_MESSAGE(ini.GetSectionSize("MY_FANTASTIC_SOFTWARE") == 2,
						"Section [MY_FANTASTIC_SOFTWARE] has 2 elements");
}
#else
BOOST_AUTO_TEST_CASE(mock) { BOOST_CHECKPOINT("Mock test for older boost versions"); }
#endif
