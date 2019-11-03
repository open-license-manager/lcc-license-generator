#define BOOST_TEST_MODULE test_product
//#define BOOST_TEST_MAIN
//#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <build_properties.h>

#include "../src/license_generator/product.hpp"
#include "../src/ini/SimpleIni.h"
#include "../src/base_lib/base.h"

namespace fs = boost::filesystem;
using namespace license;
using namespace std;

BOOST_AUTO_TEST_CASE( product_initialize ) {
	const fs::path mock_source_folder(
			fs::path(PROJECT_TEST_SRC_DIR) / "data" / "src");
	const fs::path project_folder(
			fs::path(PROJECT_TEST_TEMP_DIR) / "product_initialize");
	const fs::path expectedPrivateKey(project_folder / PRIVATE_KEY_FNAME);

	fs::remove_all(project_folder);
	BOOST_CHECK_MESSAGE(!fs::exists(expectedPrivateKey),
			"Private key " + expectedPrivateKey.string() + " doesn't exist.");
	Project prj("TEST", project_folder.string(), mock_source_folder.string(),
			false);
	prj.initialize();
	BOOST_CHECK_MESSAGE(fs::exists(expectedPrivateKey),
			"Private key " + expectedPrivateKey.string() + " created.");
}

BOOST_AUTO_TEST_CASE( product_initialize_force ) {

}
