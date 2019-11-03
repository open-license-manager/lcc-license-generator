/*
 * CryptoHelperWindows.h
 *
 *  Created on: Sep 14, 2014
 *
 */

#ifndef CRYPTOHELPERWINDOWS_H_
#define CRYPTOHELPERWINDOWS_H_

#include <windows.h>
#include <wincrypt.h>
#include <tchar.h>
#include <string>
#include "../crypto_helper.hpp"

namespace license {
using namespace std;

class CryptoHelperWindows: public CryptoHelper {
private:
	//	Handle to the private key.
	BCRYPT_KEY_HANDLE m_hTmpKey = nullptr;
	const BCRYPT_ALG_HANDLE m_hSignAlg;
	BCRYPT_ALG_HANDLE m_hHashAlg = nullptr;
	void printHash(HCRYPTHASH *hHash) const;

public:
	CryptoHelperWindows();

	virtual void generateKeyPair();
	const virtual string exportPrivateKey() const;
	const virtual string exportPublicKey() const;
	virtual void loadPrivateKey(const std::string &privateKey);
	const virtual string signString(const string &license) const;

	virtual ~CryptoHelperWindows();
};

} /* namespace license */

#endif /* CRYPTOHELPERWINDOWS_H_ */
