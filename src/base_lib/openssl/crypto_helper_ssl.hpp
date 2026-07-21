/*
 * CryptpHelperLinux.h
 *
 *  Created on: Sep 14, 2014
 *
 */

#ifndef CRYPTPHELPERLINUX_H_
#define CRYPTPHELPERLINUX_H_

#include <openssl/evp.h>
#include <cstddef>
#include <string>
#include <vector>
#include "build_properties.h"
#include "../crypto_helper.hpp"

namespace license {
using namespace std;

class CryptoHelperLinux : public CryptoHelper {
private:
	static const int kExp = 65537;
	EVP_PKEY *m_pktmp;
	const string Opensslb64Encode(const size_t slen, const unsigned char *signature) const;

public:
	CryptoHelperLinux();
	// disable copy constructor
	CryptoHelperLinux(const CryptoHelperLinux &) = delete;

	virtual void generateKeyPair(int keyBits = DEFAULT_RSA_KEY_BITS);
	const virtual string exportPrivateKey() const;
	const virtual std::vector<unsigned char> exportPublicKey() const;
	virtual void loadPrivateKey(const std::string &privateKey);
	const virtual string signString(const string &stringToBeSigned) const;
	virtual unsigned int privateKeyBits() const;
	virtual ~CryptoHelperLinux();
};

} /* namespace license */

#endif /* CRYPTPHELPERLINUX_H_ */
