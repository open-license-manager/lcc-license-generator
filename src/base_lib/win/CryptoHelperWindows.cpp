/*
 * CryptoHelperWindows.cpp
 *
 *  Created on: Sep 14, 2014
 *
 */

#include <sstream>
#include <vector>
#include <string>

#include <bcrypt.h>
#include <ncrypt.h>
#include <wincrypt.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

#include "CryptoHelperWindows.h"
#include <iostream>

namespace license {
using namespace std;
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)

static string formatError(DWORD status) {
	vector<char> msgBuffer(256);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
	NULL, status, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &msgBuffer[0],
			sizeof(msgBuffer) - 1, nullptr);
	return string(&msgBuffer[0]);
}

static BCRYPT_ALG_HANDLE openSignatureProvider() {
	DWORD status;
	BCRYPT_ALG_HANDLE hSignAlg = nullptr;
	if (!NT_SUCCESS(
			status = BCryptOpenAlgorithmProvider( &hSignAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) {
		wprintf(L"**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n",
				status);
		throw logic_error("Error opening signature provider");
	}
	return hSignAlg;
}

CryptoHelperWindows::CryptoHelperWindows() :
		m_hSignAlg(openSignatureProvider()) {
	DWORD status;
	if (!NT_SUCCESS(
			status = BCryptOpenAlgorithmProvider( &m_hHashAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0))) {
		wprintf(L"**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n",
				status);
		throw logic_error(string("Error finding hash algorigthm. "));
	}
}

/**
 This method calls the CryptGenKey function to get a handle to an

 exportable key-pair. The key-pair is  generated with the RSA public-key
 key exchange algorithm using Microsoft Enhanced Cryptographic Provider.
 */
void CryptoHelperWindows::generateKeyPair() {
	HRESULT hr = S_OK;
	DWORD dwErrCode;
	// If the handle to key container is NULL, fail.
	if (m_hCryptProv == NULL)
		throw logic_error("Cryptocontext not correctly initialized");
	// Release a previously acquired handle to key-pair.
	if (m_hCryptKey) {
		m_hCryptKey = NULL;
	}
	// Call the CryptGenKey method to get a handle
	// to a new exportable key-pair.
	if (!CryptGenKey(m_hCryptProv, CALG_RSA_SIGN,
			RSA1024BIT_KEY | CRYPT_EXPORTABLE, &m_hCryptKey)) {
		dwErrCode = GetLastError();
		throw logic_error(
				string("Error generating keys ")
						+ to_string(static_cast<long long>(dwErrCode)));
	}
	//double check the key is really generated
	if (m_hCryptKey == NULL) {
		dwErrCode = GetLastError();
		throw logic_error(
				string("Error generating keys (2)")
						+ to_string(static_cast<long long>(dwErrCode)));
	}
}

/* This method calls the CryptExportKey function to get the Public key
 in a string suitable for C source inclusion.
 */
const string CryptoHelperWindows::exportPublicKey() const {
	HRESULT hr = S_OK;
	DWORD dwErrCode;
	DWORD dwBlobLen;
	BYTE *pbKeyBlob = nullptr;
	stringstream ss;
	// If the handle to key container is NULL, fail.
	if (m_hCryptKey == NULL)
		throw logic_error("call GenerateKey first.");
	// This call here determines the length of the key
	// blob.
	if (!CryptExportKey(m_hCryptKey,
	NULL, PUBLICKEYBLOB, 0, nullptr, &dwBlobLen)) {
		dwErrCode = GetLastError();
		throw logic_error(
				string("Error calculating size of public key ")
						+ to_string(static_cast<long long>(dwErrCode)));
	}
	// Allocate memory for the pbKeyBlob.
	if ((pbKeyBlob = new BYTE[dwBlobLen]) == nullptr) {
		throw logic_error(string("Out of memory exporting public key "));
	}
	// Do the actual exporting into the key BLOB.
	if (!CryptExportKey(m_hCryptKey,
	NULL, PUBLICKEYBLOB, 0, pbKeyBlob, &dwBlobLen)) {
		delete pbKeyBlob;
		dwErrCode = GetLastError();
		throw logic_error(
				string("Error exporting public key ")
						+ to_string(static_cast<long long>(dwErrCode)));
	} else {
		ss << "\t";
		for (unsigned int i = 0; i < dwBlobLen; i++) {
			if (i != 0) {
				ss << ", ";
				if (i % 10 == 0) {
					ss << "\\" << endl << "\t";
				}
			}
			ss << to_string(static_cast<long long>(pbKeyBlob[i]));
		}
		delete pbKeyBlob;
	}
	return ss.str();
}

CryptoHelperWindows::~CryptoHelperWindows() {
	BCryptCloseAlgorithmProvider(m_hHashAlg, 0);
	BCryptCloseAlgorithmProvider(m_hSignAlg, 0);
}

const string CryptoHelperWindows::exportPrivateKey() const {
	HRESULT hr = S_OK;
	DWORD dwErrCode;
	DWORD dwBlobLen;
	BYTE *pbKeyBlob;
	stringstream ss;
	// If the handle to key container is NULL, fail.
	if (m_hCryptKey == NULL)
		throw logic_error(string("call GenerateKey first."));
	// This call here determines the length of the key
	// blob.
	if (!CryptExportKey(m_hCryptKey,
	NULL, PRIVATEKEYBLOB, 0, nullptr, &dwBlobLen)) {
		dwErrCode = GetLastError();
		throw logic_error(
				string("Error calculating size of private key ")
						+ to_string(static_cast<long long>(dwErrCode)));
	}
	// Allocate memory for the pbKeyBlob.
	if ((pbKeyBlob = new BYTE[dwBlobLen]) == nullptr) {
		throw logic_error(string("Out of memory exporting private key "));
	}

	// Do the actual exporting into the key BLOB.
	if (!CryptExportKey(m_hCryptKey,
	NULL, PRIVATEKEYBLOB, 0, pbKeyBlob, &dwBlobLen)) {
		delete pbKeyBlob;
		dwErrCode = GetLastError();
		throw logic_error(
				string("Error exporting private key ")
						+ to_string(static_cast<long long>(dwErrCode)));
	} else {
		ss << "\t";
		for (unsigned int i = 0; i < dwBlobLen; i++) {
			if (i != 0) {
				ss << ", ";
				if (i % 15 == 0) {
					ss << "\\" << endl << "\t";
				}
			}
			ss << to_string(static_cast<long long>(pbKeyBlob[i]));
		}
		delete pbKeyBlob;
	}
	return ss.str();
}

void CryptoHelperWindows::printHash(HCRYPTHASH *hHash) const {
	BYTE *pbHash;
	DWORD dwHashLen;
	DWORD dwHashLenSize = sizeof(DWORD);
	char *hashStr;
	unsigned int i;

	if (CryptGetHashParam(*hHash, HP_HASHSIZE, (BYTE*) &dwHashLen,
			&dwHashLenSize, 0)) {
		pbHash = (BYTE*) malloc(dwHashLen);
		hashStr = (char*) malloc(dwHashLen * 2 + 1);
		if (CryptGetHashParam(*hHash, HP_HASHVAL, pbHash, &dwHashLen, 0)) {
			for (i = 0; i < dwHashLen; i++) {
				sprintf(&hashStr[i * 2], "%02x", pbHash[i]);
			}
			printf("hash To be signed: %s \n", hashStr);
		}
		free(pbHash);
		free(hashStr);
	}
}

void CryptoHelperWindows::loadPrivateKey(const std::string &privateKey) {
	DWORD dwBufferLen = 0, cbKeyBlob = 0, cbSignature = 0, status;
	BOOL success;
	LPBYTE pbBuffer = nullptr, pbKeyBlob = NULL;
	string error;

	m_hSignAlg = openSignatureProvider();

	if (CryptStringToBinaryA(privateKey.c_str(), 0, CRYPT_STRING_BASE64HEADER,
	NULL, &dwBufferLen, NULL, NULL)) {
		pbBuffer = (LPBYTE) LocalAlloc(0, dwBufferLen);
		if (CryptStringToBinaryA(privateKey.c_str(), 0,
				CRYPT_STRING_BASE64HEADER, pbBuffer, &dwBufferLen, NULL,
				NULL)) {
			if (CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
					PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, NULL,
					&cbKeyBlob)) {
				pbKeyBlob = (LPBYTE) LocalAlloc(0, cbKeyBlob);
				success = CryptDecodeObjectEx(
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL,
						pbKeyBlob, &cbKeyBlob);
				LocalFree(pbBuffer);
				pbBuffer = nullptr;
				if (success) {
					status = BCryptImportKeyPair(m_hSignAlg, NULL,
							LEGACY_RSAPRIVATE_BLOB, &m_hTmpKey, pbKeyBlob,
							cbKeyBlob, 0);
					LocalFree(pbKeyBlob);
					if (!NT_SUCCESS(status)) {
						error = formatError(status);
						cerr << "Failed to load BASE64 private key." << error
								<< endl;
						throw logic_error(
								string("Error failed to load private key. ")
										+ error);
					}
				} else {
					error = formatError(status);
					LocalFree(pbKeyBlob);
					cerr << "Failed to convert BASE64 private key." << error
							<< endl;
					throw logic_error(
							string("Error during loadPrivateKey. ") + error);
				}
			}
		}
	}
	error = formatError(GetLastError());
	if (pbBuffer) {
		LocalFree(pbBuffer);
	}
	cerr << "Failed to load BASE64 private key." << error << endl;
	throw logic_error(string("Error during loadPrivateKey. ") + error);
}

static bool hashData(BCRYPT_HASH_HANDLE &hHash, const string &data,
		string &error, PBYTE pbHash, DWORD hashDataLenght) {
	DWORD status;
	boolean success = false;
	if (NT_SUCCESS(
			status = BCryptHashData(hHash, (BYTE* )data.c_str(), data.length(),
					0))) {
		if (NT_SUCCESS(
				status = BCryptFinishHash(hHash, pbHash, hashDataLenght, 0))) {
			success = true;
		}
	}
	if (!success) {
		error = "Error hashing data. " + formatError(status);
	}
	return success;
}

static bool signData(BCRYPT_KEY_HANDLE m_hTmpKey, PBYTE pbHash,
		DWORD hashDataLenght, string &error, string &signatureBuffer) {
	DWORD status, cbSignature;
	bool success = false;
	PBYTE pbSignature = nullptr, pbB64signature = nullptr;

	BCRYPT_PKCS1_PADDING_INFO paddingInfo;
	ZeroMemory(&paddingInfo, sizeof(paddingInfo));
	paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

	if (NT_SUCCESS(
			status = BCryptSignHash( m_hTmpKey, &paddingInfo, pbHash, hashDataLenght, NULL, 0, &cbSignature, BCRYPT_PAD_PKCS1))) {
		pbSignature = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbSignature);
		if (NULL != pbSignature) {
			if (NT_SUCCESS(
					status = BCryptSignHash(m_hTmpKey, &paddingInfo, pbHash,
							hashDataLenght, pbSignature, cbSignature,
							&cbSignature, BCRYPT_PAD_PKCS1))) {
				DWORD final;
				CryptBinaryToString(pbSignature, cbSignature,
						CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr,
						&final);
				pbB64signature = (PBYTE) HeapAlloc(GetProcessHeap(), 0,
						cbSignature);
				CryptBinaryToString(pbSignature, cbSignature,
						CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
						pbB64signature, &final);
				signatureBuffer.append((char*) pbB64signature, final);
				success = true;
			} else {
				error = "**** signature failed";
			}
		} else {
			error = "**** memory allocation failed";
		}
	}

	//allocate the signature buffer
	return success;

}

const string CryptoHelperWindows::signString(const string &license) const {

	string error;
	DWORD status;
	BCRYPT_HASH_HANDLE hHash;
	BYTE *pbSignature;
	string signatureBuffer;
	PBYTE pbHashObject = nullptr, pbHashData = nullptr;
	bool success = false;
	//calculate the size of the buffer to hold the hash object
	DWORD cbData = 0, cbHashObject = 0;
	//and the size to keep the hashed data
	DWORD cbHashDataLenght = 0;
	if (NT_SUCCESS(
			status =
					BCryptGetProperty(m_hHashAlg, BCRYPT_OBJECT_LENGTH,
							(PBYTE) & cbHashObject, sizeof(DWORD), &cbData,
							0)) && NT_SUCCESS(status = BCryptGetProperty(m_hHashAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHashDataLenght, sizeof(DWORD), &cbData, 0))) {
		//allocate the hash object on the heap
		pbHashObject = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbHashObject);
		pbHashData = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbHashDataLenght);
		if (NULL != pbHashObject && nullptr != pbHashData) {
			//create a hash
			if (NT_SUCCESS(
					status = BCryptCreateHash(m_hHashAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0))) {
				success
						== hashData(hHash, license, error, pbHashData,
								cbHashDataLenght)
						&& signData(m_hTmpKey, pbHashData, cbHashDataLenght,
								error, signatureBuffer);
			} else {
				error = "error creating hash";
			}
		} else {
			error = "**** memory allocation failed";
		}
	} else {
		error = "**** Error returned by BCryptGetProperty"
				+ formatError(status);
	}

	if (hHash) {
		BCryptDestroyHash(hHash);
	}
	if (pbHashObject) {
		HeapFree(GetProcessHeap(), 0, pbHashObject);
	}
	if (pbHashData) {
		HeapFree(GetProcessHeap(), 0, pbHashData);
	}
	if (!success) {

	}
	return signatureBuffer;
}
} /* namespace license */
