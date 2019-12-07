#define BOOST_TEST_MODULE test_cryptohelper

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <boost/filesystem.hpp>

#include <build_properties.h>
#include "../src/base_lib/crypto_helper.hpp"
#include "../src/base_lib/base.h"
#define SIGNATURE                                          \
	"0pBQSdgwE6amOQJ1T+byZhJetVl86OWLHC+ICJ/IENVoNqcJF2pD" \
	"aoRuNtDEq5v/lqmQbQJg4d08VtRCen3Q3VuUrge2e7hQ3ktkkK8"  \
	"DwTtUJA+pcB540sofcdbXabF+L+vwmj5jUWsamJzp/fhg8xpQ72L54UzjcbKsGVgsc2Y="
#define PUBKEY                                                                                                         \
	{                                                                                                                  \
		48, 129, 137, 2, 129, 129, 0, 242, 27, 37, 44, 100, 25, 53, 107, 167, 151, 101, 105, 53, 119, 68, 227, 137,    \
			62, 246, 187, 227, 178, 59, 225, 20, 142, 0, 56, 55, 116, 45, 49, 162, 188, 82, 33, 155, 220, 4, 169, 49,  \
			33, 41, 65, 178, 196, 44, 191, 232, 167, 5, 94, 182, 158, 245, 5, 116, 79, 247, 201, 162, 218, 114, 209,   \
			244, 247, 215, 73, 89, 239, 242, 161, 210, 117, 236, 188, 216, 193, 212, 143, 58, 153, 6, 213, 171, 39,    \
			166, 127, 48, 234, 167, 232, 161, 212, 66, 141, 198, 93, 235, 88, 210, 38, 172, 25, 109, 107, 153, 133, 0, \
			231, 128, 203, 216, 110, 161, 24, 230, 50, 152, 74, 215, 115, 246, 146, 152, 193, 20, 209, 2, 3, 1, 0, 1   \
	}

namespace fs = boost::filesystem;
using namespace license;
using namespace std;

namespace test {

const std::string loadPrivateKey() {
	fs::path pkf = fs::path(PROJECT_TEST_SRC_DIR) / "data" / PRIVATE_KEY_FNAME;
	std::ifstream private_key_linux(pkf.string());
	BOOST_REQUIRE_MESSAGE(private_key_linux.good(), "test file found");
	const std::string pk_str((std::istreambuf_iterator<char>(private_key_linux)), std::istreambuf_iterator<char>());
	return pk_str;
}

BOOST_AUTO_TEST_CASE(test_generate_and_sign) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->generateKeyPair();
	const string privateK = crypto->exportPrivateKey();
	BOOST_CHECK_MESSAGE(boost::starts_with(privateK, "-----BEGIN RSA PRIVATE KEY-----"),
						"Private key is in openssl pkcs#1 format");
	const std::string signature = crypto->signString("testString");
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
	crypto.release();
	/*
	 ofstream myfile("private_key-linux.rsa");
	 myfile << privateK;
	 myfile.close();*/
}

/**
 * Import a private key, export it again and check imported and exported are equal
 */
BOOST_AUTO_TEST_CASE(test_load_and_export_private) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	const std::string pk_str = loadPrivateKey();
	crypto->loadPrivateKey(pk_str);
	std::string pk_exported = crypto->exportPrivateKey();
	// cout<<pk_str<<endl;
	// cout<<pk_exported<<endl;
	BOOST_CHECK_MESSAGE(boost::trim_copy(pk_exported) == boost::trim_copy(pk_str),
						"imported and exported keys are the same");
	crypto.release();
}

BOOST_AUTO_TEST_CASE(test_load_and_export_public_key) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	const vector<unsigned char> expected_pubkey(PUBKEY);
	const std::string pk_str = loadPrivateKey();
	crypto->loadPrivateKey(pk_str);
	vector<unsigned char> pk_exported = crypto->exportPublicKey();

	/*
	 for (auto it : pk_exported) {
	 cout << ((int)it) << ",";
	 }
	 ofstream myfile("public_key.rsa");
	 for (auto it : pk_exported) {
	 myfile << it;
	 }
	 myfile.close();*/
	BOOST_CHECK_MESSAGE(expected_pubkey.size() == pk_exported.size(), "exported key and expected are the same size");
	BOOST_CHECK_MESSAGE(std::equal(expected_pubkey.begin(), expected_pubkey.end(), pk_exported.begin()),
						"exported key and expected have the same content");
	crypto.release();
}

BOOST_AUTO_TEST_CASE(test_load_and_sign) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	const std::string pk_str = loadPrivateKey();
	crypto->loadPrivateKey(pk_str);
	const std::string signature = crypto->signString("testString");
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
	BOOST_CHECK_MESSAGE(signature == SIGNATURE, "signature is repeatable");
	crypto.release();
}

BOOST_AUTO_TEST_CASE(test_generate_export_import_and_sign) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->generateKeyPair();
	const string pk = crypto->exportPrivateKey();
	crypto->loadPrivateKey(pk);
	const string signature = crypto->signString("testString");
	//(1024/8)*(4/3)+4 (base64)
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
	crypto.release();
}
}  // namespace test
