/*
 * CryptoHelperWindows.h
 *
 *  Created on: Sep 14, 2014
 *
 */

#ifndef CRYPTOHELPERWINDOWS_H_
#define CRYPTOHELPERWINDOWS_H_

#include <windows.h>
#include <bcrypt.h>
#include <string>
#include "../crypto_helper.hpp"

namespace license {
using namespace std;

class CryptoHelperWindows : public CryptoHelper {
private:
	//	Handle to the private key.
	BCRYPT_KEY_HANDLE m_hTmpKey = nullptr;
	const BCRYPT_ALG_HANDLE m_hSignAlg;
	const BCRYPT_ALG_HANDLE m_hHashAlg;

public:
	CryptoHelperWindows();
	CryptoHelperWindows(const CryptoHelperWindows &) = delete;

	virtual void generateKeyPair();
	/*
	 * exports the private key in openssl pkcs#1 PEM encoded format.
	 */
	const virtual string exportPrivateKey() const;
	const virtual vector<unsigned char> exportPublicKey() const;
	/*
	 * loads a private key in openssl pkcs#1 PEM encoded format.
	 */
	virtual void loadPrivateKey(const std::string &privateKey);
	const virtual string signString(const string &license) const;

	virtual ~CryptoHelperWindows();
};

} /* namespace license */

#endif /* CRYPTOHELPERWINDOWS_H_ */
