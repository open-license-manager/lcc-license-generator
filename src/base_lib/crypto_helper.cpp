#include <memory>
#include <boost/filesystem.hpp>
#include <fstream>

#include "crypto_helper.hpp"
#ifdef __linux__
#include "linux/CryptoHelperLinux.h"
#elif _WIN32
#include "win/CryptoHelperWindows.h"
#endif

namespace license {
using namespace std;
namespace fs = boost::filesystem;

unique_ptr<CryptoHelper> CryptoHelper::getInstance() {
#ifdef __linux__
	unique_ptr<CryptoHelper> ptr((CryptoHelper *)new CryptoHelperLinux());
#elif _WIN32
	unique_ptr<CryptoHelper> ptr((CryptoHelper *)new CryptoHelperWindows());
#endif
	return ptr;
}

void CryptoHelper::loadPrivateKey_file(const std::string &privateKey_file_name) {
	if (!fs::exists(privateKey_file_name)) {
		throw logic_error("Private key file [" + privateKey_file_name + "] does not exists");
	}
	std::ifstream private_key(privateKey_file_name);
	std::string pk_string((std::istreambuf_iterator<char>(private_key)), std::istreambuf_iterator<char>());
	loadPrivateKey(pk_string);
}

}  // namespace license
