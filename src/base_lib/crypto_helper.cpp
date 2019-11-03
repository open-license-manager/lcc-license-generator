#include <memory>
#include "crypto_helper.hpp"
#ifdef __linux__
#include"linux/CryptoHelperLinux.h"
#elif _WIN32
#include"win/CryptoHelperWindows.h"
#endif

namespace license {
using namespace std;

unique_ptr<CryptoHelper> CryptoHelper::getInstance(
		const std::string &product_name) {
#ifdef __linux__
	unique_ptr<CryptoHelper> ptr((CryptoHelper*) new CryptoHelperLinux());
#elif _WIN32
	unique_ptr<CryptoHelper> ptr((CryptoHelper*) new CryptoHelperWindows(product_name));
#endif
	return ptr;
}
}

