#define BOOST_TEST_MODULE test_cryptohelper

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
// #include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <regex>
#include <boost/filesystem.hpp>

#include <build_properties.h>
#include "../src/base_lib/crypto_helper.hpp"
#include "../src/base_lib/base.h"

#define SIGNATURE                                          \
	"0pBQSdgwE6amOQJ1T+byZhJetVl86OWLHC+ICJ/IENVoNqcJF2pD" \
	"aoRuNtDEq5v/lqmQbQJg4d08VtRCen3Q3VuUrge2e7hQ3ktkkK8"  \
	"DwTtUJA+pcB540sofcdbXabF+L+vwmj5jUWsamJzp/fhg8xpQ72L54UzjcbKsGVgsc2Y="
#define PUBKEY                                                                                           \
	{48,  129, 137, 2,	 129, 129, 0,	242, 27,  37,  44,	100, 25,  53,  107, 167, 151, 101, 105, 53,  \
	 119, 68,  227, 137, 62,  246, 187, 227, 178, 59,  225, 20,	 142, 0,   56,	55,	 116, 45,  49,	162, \
	 188, 82,  33,	155, 220, 4,   169, 49,	 33,  41,  65,	178, 196, 44,  191, 232, 167, 5,   94,	182, \
	 158, 245, 5,	116, 79,  247, 201, 162, 218, 114, 209, 244, 247, 215, 73,	89,	 239, 242, 161, 210, \
	 117, 236, 188, 216, 193, 212, 143, 58,	 153, 6,   213, 171, 39,  166, 127, 48,	 234, 167, 232, 161, \
	 212, 66,  141, 198, 93,  235, 88,	210, 38,  172, 25,	109, 107, 153, 133, 0,	 231, 128, 203, 216, \
	 110, 161, 24,	230, 50,  152, 74,	215, 115, 246, 146, 152, 193, 20,  209, 2,	 3,	  1,   0,	1}

namespace fs = boost::filesystem;
using namespace license;
using namespace std;

namespace test {

const std::string loadPKfromFile(int keyBits) {
	fs::path pkf = fs::path(PROJECT_TEST_SRC_DIR) / "data" / ("pkey" + std::to_string(keyBits) + ".txt");
	std::ifstream private_key_linux(pkf.string());
	BOOST_REQUIRE_MESSAGE(private_key_linux.good(), "test file found");
	const std::string pk_str((std::istreambuf_iterator<char>(private_key_linux)), std::istreambuf_iterator<char>());
	private_key_linux.close();
	return boost::trim_copy(pk_str);
}

const vector<unsigned char> loadPublicKey(int keyBits) {
	fs::path pkf = fs::path(PROJECT_TEST_SRC_DIR) / "data" / ("pubkey" + std::to_string(keyBits) + ".bin");
	std::ifstream public_key_linux(pkf.string(), std::ios::binary);
	BOOST_REQUIRE_MESSAGE(public_key_linux.good(), "test file found");
	vector<unsigned char> pk_data((std::istreambuf_iterator<char>(public_key_linux)), std::istreambuf_iterator<char>());
	public_key_linux.close();
	return pk_data;
}

BOOST_AUTO_TEST_CASE(test_generate_and_sign_1024_bit) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->generateKeyPair(1024);
	const string privateK = crypto->exportPrivateKey();
	BOOST_CHECK_MESSAGE(boost::starts_with(privateK, "-----BEGIN RSA PRIVATE KEY-----"),
						"Private key is in openssl pkcs#1 format");
	const std::string signature = crypto->signString("testString");
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
	crypto.release();
}

BOOST_AUTO_TEST_CASE(test_generate_and_sign_4096_bit) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	crypto->generateKeyPair(4096);
	const string privateK = crypto->exportPrivateKey();
	BOOST_CHECK_MESSAGE(boost::starts_with(privateK, "-----BEGIN RSA PRIVATE KEY-----"),
						"Private key is in openssl pkcs#1 format");
	const std::string signature = crypto->signString("testString");
	BOOST_CHECK_MESSAGE(signature.size() == 684, "signature is the right size (b64 encoding of 512 bytes)");
	crypto.release();
}

BOOST_AUTO_TEST_CASE(test_load_and_export_public_key_1024) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	const vector<unsigned char> expected_pubkey(PUBKEY);
	const std::string pk_str = loadPKfromFile(1024);
	crypto->loadPrivateKey(pk_str);
	BOOST_CHECK_MESSAGE(crypto->privateKeyBits() == 1024, "Private key size [" +
															  std::to_string(crypto->privateKeyBits()) +
															  "] is different from the expected sizefor key size 1024");
	vector<unsigned char> pk_exported = crypto->exportPublicKey();
	BOOST_CHECK_MESSAGE(expected_pubkey.size() == pk_exported.size(), "exported key and expected are the same size");
	BOOST_CHECK_MESSAGE(std::equal(expected_pubkey.begin(), expected_pubkey.end(), pk_exported.begin()),
						"exported key and expected have the same content");
	crypto.release();
}
/*
Load private key, re-export it, export the public key and compare with the reference.
*/
BOOST_AUTO_TEST_CASE(test_load_and_export) {
	const std::vector<int> keySizes = {1024, 2048, 4096};
	for (const auto& keySize : keySizes) {
		unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
		const std::string disk_pk = loadPKfromFile(keySize);
		const vector<unsigned char> pubkey_on_disk = loadPublicKey(keySize);
		crypto->loadPrivateKey(disk_pk);
		vector<unsigned char> pubkey_exported = crypto->exportPublicKey();
		// check public key
		BOOST_CHECK_MESSAGE(pubkey_on_disk.size() == pubkey_exported.size(),
							"exported key and expected are the same size");
		BOOST_CHECK_MESSAGE(std::equal(pubkey_on_disk.begin(), pubkey_on_disk.end(), pubkey_exported.begin()),
							"exported key and expected have the same content");
		// check private key
		BOOST_CHECK_MESSAGE(crypto->privateKeyBits() == keySize,
							"Private key size [" + std::to_string(crypto->privateKeyBits()) +
								"] is different from the expected size for key size " + std::to_string(keySize));
		std::string pk_exported = crypto->exportPrivateKey();
		std::string exported_norm = std::regex_replace(pk_exported, std::regex("\r\n|\r"), "\n");
		string exported = boost::trim_copy(exported_norm);
		BOOST_CHECK_MESSAGE(exported == disk_pk, "imported and exported private keys are NOT the same");
		crypto.release();
	}
}

BOOST_AUTO_TEST_CASE(test_repeatable_signature) {
	unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
	const std::string pk_str = loadPKfromFile(1024);
	crypto->loadPrivateKey(pk_str);

	const std::string signature = crypto->signString("testString");
	unsigned int keySize = crypto->privateKeyBits();
	BOOST_CHECK_MESSAGE(keySize == 1024, "Using old 1024 bit " + std::to_string(keySize));
	BOOST_CHECK_MESSAGE(signature.size() == 172, "signature is the right size");
	BOOST_CHECK_MESSAGE(signature == SIGNATURE, "signature is repeatable");
	crypto.release();
}

BOOST_AUTO_TEST_CASE(test_generate_export_import_and_sign) {
	const std::vector<int> keySizes = {1024, 2048, 4096};
	for (const auto& keySize : keySizes) {
		unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
		crypto->generateKeyPair(keySize);
		const string pk = crypto->exportPrivateKey();
		crypto->loadPrivateKey(pk);
		const string signature = crypto->signString("testString");
		int exp_size = ((keySize / 8) + 2) / 3 * 4;	 // base64 encoding of keySize/8 bytes
		BOOST_CHECK_MESSAGE(signature.size() == exp_size, "signature lenght [" + std::to_string(signature.size()) +
															  "] is different from the expected size [" +
															  std::to_string(exp_size) + "] for key size " +
															  std::to_string(keySize));
		crypto.release();
	}
}

BOOST_AUTO_TEST_CASE(test_generate_and_check_len) {
	const std::vector<int> keySizes = {1024, 2048, 4096};
	for (const auto& keySize : keySizes) {
		unique_ptr<CryptoHelper> crypto(CryptoHelper::getInstance());
		crypto->generateKeyPair(keySize);
		BOOST_CHECK_MESSAGE(crypto->privateKeyBits() == keySize,
							"Private key size [" + std::to_string(crypto->privateKeyBits()) +
								"] is different from the expected sizefor key size " + std::to_string(keySize));
		/*ofstream outfile("pkey"+std::to_string(keySize)+".txt", ofstream::binary);
		outfile << crypto->exportPrivateKey();
		outfile.close();
		ofstream outfile1("pubkey"+std::to_string(keySize)+".txt", ofstream::binary);
		vector<unsigned char> data = crypto->exportPublicKey();
		copy(data.cbegin(), data.cend(), ostreambuf_iterator<char>(outfile1));
		outfile1.close();*/
		crypto.release();
	}
}
}  // namespace test