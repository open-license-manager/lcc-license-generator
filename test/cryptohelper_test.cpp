#define BOOST_TEST_MODULE test_cryptohelper

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <boost/filesystem.hpp>

#include <build_properties.h>
#include "../src/base_lib/crypto_helper.hpp"
#include "../src/base_lib/base.h"
#define SIGNATURE  "0pBQSdgwE6amOQJ1T+byZhJetVl86OWLHC+ICJ/IENVoNqcJF2pD" \
					"aoRuNtDEq5v/lqmQbQJg4d08VtRCen3Q3VuUrge2e7hQ3ktkkK8" \
					"DwTtUJA+pcB540sofcdbXabF+L+vwmj5jUWsamJzp/fhg8xpQ72L54UzjcbKsGVgsc2Y="
namespace fs = boost::filesystem;
using namespace license;
using namespace std;

namespace test {
BOOST_AUTO_TEST_CASE( test_generate_and_sign ) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance("TEST"));
	crypto->generateKeyPair();
	const string privateK = crypto->exportPrivateKey();
	BOOST_CHECK_MESSAGE(
			boost::starts_with(privateK, "-----BEGIN PRIVATE KEY-----"),
			"Private key is in openssl pkcs#8 format");
	const std::string signature = crypto->signString("testString");
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
/*
	 ofstream myfile("private_key-linux.rsa");
	  myfile << privateK;
	  myfile.close();*/
}

BOOST_AUTO_TEST_CASE( test_load_and_sign ) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance("TEST_LOAD"));
	fs::path pkf = fs::path(PROJECT_TEST_SRC_DIR) / "data"
			/ "private_key-linux.rsa";
	std::ifstream private_key_linux(pkf.string());
	BOOST_REQUIRE_MESSAGE(private_key_linux.good(), "test file found");
	const std::string pk_str(
			(std::istreambuf_iterator<char>(private_key_linux)),
			std::istreambuf_iterator<char>());

	crypto->loadPrivateKey(pk_str);
	const std::string signature = crypto->signString("testString");
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
	BOOST_CHECK_MESSAGE(signature == SIGNATURE, "signature is repeatable");
}

BOOST_AUTO_TEST_CASE( test_generate_export_import_and_sign ) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance("TEST_EXP"));
	crypto->generateKeyPair();
	const string pk = crypto->exportPrivateKey();
	crypto->loadPrivateKey(pk);
	const string signature = crypto->signString("testString");
	//(1024/8)*(4/3)+4 (base64)
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
}
}

