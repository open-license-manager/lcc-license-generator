/*
 * CryptoHelperWindows.cpp
 *
 *  Created on: Sep 14, 2014
 *
 */

#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <locale>

#include <windows.h>
#include <windef.h>
#include <bcrypt.h>
#include <ncrypt.h>
#include <wincrypt.h>
#include <fstream>
#include <math.h>
#include <boost/algorithm/string/predicate.hpp>

#include "../base64.h"
#include "CryptoHelperWindows.h"

// #pragma comment(lib, "bcrypt.lib")
// #pragma comment(lib, "crypt32.lib")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

namespace license {
using namespace std;

static const string formatError(DWORD status) {
	std::ostringstream ss;
	ss << std::hex << status;
	/* vector<char> msgBuffer(256);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &msgBuffer[0],
				  sizeof(msgBuffer) - 1, nullptr);
	return string(&msgBuffer[0]) + ss.str();*/

	LPSTR messageBuffer = nullptr;

	// Ask Windows to allocate the buffer and fetch the system error text
	DWORD size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
									FORMAT_MESSAGE_IGNORE_INSERTS,	// Safe: ignores embedded %1, %2 placeholders
								NULL,  // No external module needed for system errors
								status,	 // The error code (e.g., from GetLastError())
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
								reinterpret_cast<LPSTR>(&messageBuffer),  // Polymorphic pointer trick
								0,	// Minimum size to allocate
								NULL  // No arguments needed due to IGNORE_INSERTS
	);

	if (size == 0) {
		return string("Unknown error code: ") + ss.str();
	}

	// Copy to a modern safe string container
	string errorMessage(messageBuffer, size);
	LocalFree(messageBuffer);
	return errorMessage + ", error (0x" + ss.str() + ")";
}

static BCRYPT_ALG_HANDLE openSignatureProvider() {
	DWORD status;
	BCRYPT_ALG_HANDLE hSignAlg = nullptr;
	if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hSignAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) {
		cerr << "**** Error returned by BCryptOpenAlgorithmProvider" << formatError(status) << endl;
		throw logic_error("Error opening signature provider");
	}
	return hSignAlg;
}

static BCRYPT_ALG_HANDLE openHashProvider() {
	DWORD status;
	BCRYPT_ALG_HANDLE hHashAlg = nullptr;
	if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hHashAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0))) {
		cerr << "**** Error returned by BCryptOpenAlgorithmProvider" << formatError(status) << endl;
		throw logic_error("Error opening hash provider");
	}
	return hHashAlg;
}

static vector<uint8_t> export_privateKey_blob(const BCRYPT_KEY_HANDLE& m_hTmpKey) {
	DWORD status;
	DWORD dwBlobLen;
	vector<uint8_t> result;

	if (m_hTmpKey == nullptr) {
		throw logic_error(string("call GenerateKey or import a PK first."));
	}

	if (!NT_SUCCESS(status = BCryptExportKey(m_hTmpKey, nullptr, LEGACY_RSAPRIVATE_BLOB, nullptr, 0, &dwBlobLen, 0))) {
		throw logic_error(string("Error calculating size of private key ") + to_string(static_cast<long long>(status)));
	}
	// Allocate memory for the pbKeyBlob.
	result.resize(dwBlobLen);

	// Do the actual exporting into the key BLOB.
	if (!NT_SUCCESS(
			status = BCryptExportKey(m_hTmpKey, NULL, LEGACY_RSAPRIVATE_BLOB, &result[0], dwBlobLen, &dwBlobLen, 0))) {
		throw logic_error(string("Error exporting private key ") + to_string(static_cast<long long>(status)));
	}
	return result;
}

static string private_key_to_B64(const vector<uint8_t>& legacyBlob) {
	DWORD cbPem = 0;
	DWORD cbDer = 0;

	vector<BYTE> pk1_encoded;

	// 4. Query length for PKCS#1 DER encoding
	if (!CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, legacyBlob.data(), 0, NULL,
							 NULL, &cbDer)) {
		throw logic_error(string("CryptEncodeObjectEx (get size) failed: " + formatError(GetLastError())));
	}
	pk1_encoded.resize(cbDer);

	// 5. Encode CAPI blob into PKCS#1 ASN.1 DER format
	if (!CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, legacyBlob.data(), 0, NULL,
							 pk1_encoded.data(), &cbDer)) {
		throw logic_error(string("CryptEncodeObjectEx failed: " + formatError(GetLastError())));
	}

	// Query size for Base64 PEM encoding
	if (!CryptBinaryToStringA(pk1_encoded.data(), static_cast<DWORD>(pk1_encoded.size()), CRYPT_STRING_BASE64, NULL,
							  &cbPem)) {
		throw logic_error(string("CryptBinaryToStringA (len) failed: " + formatError(GetLastError())));
	}

	std::string pemString(cbPem, '\0');
	// Convert to PEM
	if (!CryptBinaryToStringA(pk1_encoded.data(), static_cast<DWORD>(pk1_encoded.size()), CRYPT_STRING_BASE64,
							  &pemString[0], &cbPem)) {
		throw logic_error(string("CryptBinaryToStringA failed: " + formatError(GetLastError())));
	}

	// Remove null terminator if included
	if (!pemString.empty() && pemString.back() == '\0') {
		pemString.pop_back();
	}

	return pemString;
}

/*******************************
Class methods (members)
*************************/

CryptoHelperWindows::CryptoHelperWindows() : m_hSignAlg(openSignatureProvider()), m_hHashAlg(openHashProvider()) {}

/**
 This method calls the BCryptGenerateKeyPair function to get a handle to an
 exportable key-pair.
 */
void CryptoHelperWindows::generateKeyPair(int keyBits) {
	DWORD status;
	if (m_hTmpKey != nullptr) {
		BCryptDestroyKey(m_hTmpKey);
		m_hTmpKey = nullptr;
	}
	if (!NT_SUCCESS(status = BCryptGenerateKeyPair(m_hSignAlg, &m_hTmpKey, (ULONG)keyBits, 0))) {
		const string err("error generating keypair" + formatError(status));
		throw logic_error(err);
	} else if (!NT_SUCCESS(status = BCryptFinalizeKeyPair(m_hTmpKey, 0))) {
		const string err("error finalizing keypair" + formatError(status));
		throw logic_error(err);
	}
}

const vector<unsigned char> CryptoHelperWindows::exportPublicKey() const {
	NTSTATUS status;
	DWORD cbLegacyBlob = 0;
	vector<unsigned char> result;

	// 1. Get size of legacy CAPI key blob
	status = BCryptExportKey(m_hTmpKey, NULL, LEGACY_RSAPUBLIC_BLOB, NULL, 0, &cbLegacyBlob, 0);
	if (status != 0) {
		throw logic_error(string("BCryptExportKey (get size) failed: " + formatError(status)));
	}

	std::vector<BYTE> legacyBlob(cbLegacyBlob);
	// 2. Export key to legacy CAPI blob format
	status = BCryptExportKey(m_hTmpKey, NULL, LEGACY_RSAPUBLIC_BLOB, legacyBlob.data(), cbLegacyBlob, &cbLegacyBlob, 0);
	if (status != 0) {
		throw logic_error(string("BCryptExportKey failed: " + formatError(status)));
	}

	DWORD cbDer = 0;
	// 4. Query length for PKCS#1 DER encoding
	if (!CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB, legacyBlob.data(), 0, NULL,
							 NULL, &cbDer)) {
		throw logic_error(string("CryptEncodeObjectEx (get size) failed: " + formatError(GetLastError())));
	}
	result.resize(cbDer);

	// 5. Encode CAPI blob into PKCS#1 ASN.1 DER format
	if (!CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB, legacyBlob.data(), 0, NULL,
							 result.data(), &cbDer)) {
		throw logic_error(string("CryptEncodeObjectEx failed: " + formatError(GetLastError())));
	}
	return result;
}

CryptoHelperWindows::~CryptoHelperWindows() {
	if (m_hTmpKey != nullptr) {
		BCryptDestroyKey(m_hTmpKey);
	}
	BCryptCloseAlgorithmProvider(m_hHashAlg, 0);
	BCryptCloseAlgorithmProvider(m_hSignAlg, 0);
}

const string CryptoHelperWindows::exportPrivateKey() const {
	stringstream ss;
	vector<uint8_t> pbKeyBlob = export_privateKey_blob(m_hTmpKey);
	string b64String = private_key_to_B64(pbKeyBlob);
	string pemString = "-----BEGIN RSA PRIVATE KEY-----\n" + b64String + "-----END RSA PRIVATE KEY-----\n";

	/* ofstream mystream;
	mystream.open("C:\\Users\\gabencoded.bin", fstream::binary | fstream::trunc);
		//for (const auto& e : encoded) mystream << e;
	mystream << pemString <<endl;
	mystream.close();
	return ss.str();*/
	return pemString;
}

void CryptoHelperWindows::loadPrivateKey(const std::string& privateKey) {
	DWORD dwBufferLen = 0, pkiLen = 0;
	LPBYTE pbBuffer = nullptr;
	PCRYPT_DER_BLOB pki = nullptr;
	string errors;

	if (m_hTmpKey != nullptr) {
		BCryptDestroyKey(m_hTmpKey);
		m_hTmpKey = nullptr;
	}

	if (!boost::starts_with(privateKey, "-----BEGIN RSA PRIVATE KEY-----")) {
		throw logic_error("Private Key is not in the right format. It must be pkcs#1 encoded PEM.");
	}
	if (CryptStringToBinaryA(privateKey.c_str(), 0, CRYPT_STRING_BASE64HEADER, NULL, &dwBufferLen, NULL, NULL)) {
		pbBuffer = (LPBYTE)LocalAlloc(0, dwBufferLen);
		if (CryptStringToBinaryA(privateKey.c_str(), 0, CRYPT_STRING_BASE64HEADER, pbBuffer, &dwBufferLen, NULL,
								 NULL)) {
			if (CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, pbBuffer,
									dwBufferLen, CRYPT_DECODE_ALLOC_FLAG, NULL, &pki, &pkiLen)) {
				if (m_hTmpKey != nullptr) {
					BCryptDestroyKey(m_hTmpKey);
					m_hTmpKey = nullptr;
				}
				DWORD status =
					BCryptImportKeyPair(m_hSignAlg, NULL, LEGACY_RSAPRIVATE_BLOB, &m_hTmpKey, (PUCHAR)pki, pkiLen, 0);
				if (NT_SUCCESS(status)) {
					LocalFree(pki);
					LocalFree(pbBuffer);
					return;
				}
				errors = "BCryptImportKeyPair " + formatError(status);
			} else {
				errors = "CryptDecodeObjectEx" + formatError(GetLastError());
			}
		}
	}
	errors += formatError(GetLastError());
	if (pbBuffer) {
		LocalFree(pbBuffer);
	}
	if (pki) {
		LocalFree(pki);
	}
	cerr << "Failed to load private key." << errors << endl;
	throw logic_error(string("Error during loadPrivateKey. ") + errors);
}

unsigned int CryptoHelperWindows::privateKeyBits() const {
	DWORD status;
	DWORD cbResult = 0;
	unsigned int pdwKeyLengthBits = 0;

	if (m_hTmpKey == nullptr) {
		throw logic_error(string("call GenerateKey or import a PK first."));
	}

	if (!NT_SUCCESS(status =
						BCryptGetProperty(m_hTmpKey, BCRYPT_KEY_LENGTH, reinterpret_cast<PUCHAR>(&pdwKeyLengthBits),
										  sizeof(DWORD), &cbResult, 0))) {
		throw logic_error(string("Error calculating size of private key " + formatError(status)));
	}
	return pdwKeyLengthBits;
}

static bool hashData(BCRYPT_HASH_HANDLE& hHash, const string& data, string& error, PBYTE pbHash, DWORD hashDataLenght) {
	DWORD status;
	bool success = false;
	if (NT_SUCCESS(status = BCryptHashData(hHash, (BYTE*)data.c_str(), (ULONG)data.length(), 0))) {
		success = NT_SUCCESS(status = BCryptFinishHash(hHash, pbHash, hashDataLenght, 0));
	}
	if (!success) {
		error = "Error hashing data. " + formatError(status);
	}
	return success;
}

static bool signData(BCRYPT_KEY_HANDLE m_hTmpKey, PBYTE pbHash, DWORD hashDataLenght, string& error,
					 string& signatureBuffer) {
	const HANDLE hProcessHeap = GetProcessHeap();
	DWORD status, cbSignature;
	bool success = false;
	PBYTE pbSignature = nullptr;

	BCRYPT_PKCS1_PADDING_INFO paddingInfo;
	ZeroMemory(&paddingInfo, sizeof(paddingInfo));
	paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

	if (NT_SUCCESS(status = BCryptSignHash(m_hTmpKey, &paddingInfo, pbHash, hashDataLenght, NULL, 0, &cbSignature,
										   BCRYPT_PAD_PKCS1))) {
		pbSignature = (PBYTE)HeapAlloc(hProcessHeap, 0, cbSignature);
		if (NULL != pbSignature) {
			if (NT_SUCCESS(status = BCryptSignHash(m_hTmpKey, &paddingInfo, pbHash, hashDataLenght, pbSignature,
												   cbSignature, &cbSignature, BCRYPT_PAD_PKCS1))) {
				DWORD finalSize = 0;
				if (CryptBinaryToString(pbSignature, cbSignature, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr,
										&finalSize)) {
					signatureBuffer.resize(
						finalSize - 1);	 // finalSize counts the \0 in the end, while string counts only the characters
					success = CryptBinaryToString(pbSignature, cbSignature, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
												  const_cast<char*>(signatureBuffer.data()), &finalSize);
					if (!success) {
						status = GetLastError();
						error = "problem exporting data " + formatError(status);
					}
				} else {
					status = GetLastError();
					error = "problem exporting data " + formatError(status);
				}
			} else {
				error = "**** signature failed " + formatError(status);
			}
		} else {
			error = "**** memory allocation failed ";
		}
	}

	if (pbSignature) {
		HeapFree(hProcessHeap, 0, pbSignature);
	}
	return success;
}

const string CryptoHelperWindows::signString(const string& license) const {
	const HANDLE hProcessHeap = GetProcessHeap();
	string error;
	DWORD status = 0;
	BCRYPT_HASH_HANDLE hHash = nullptr;
	string signatureBuffer;
	PBYTE pbHashObject = nullptr, pbHashData = nullptr;
	bool success = false;
	// calculate the size of the buffer to hold the hash object
	DWORD cbData = 0, cbHashObject = 0;
	// and the size to keep the hashed data
	DWORD cbHashDataLenght = 0;
	if (NT_SUCCESS(status = BCryptGetProperty(m_hHashAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD),
											  &cbData, 0)) &&
		NT_SUCCESS(status = BCryptGetProperty(m_hHashAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHashDataLenght, sizeof(DWORD),
											  &cbData, 0))) {
		// allocate the hash object on the heap
		pbHashObject = (PBYTE)HeapAlloc(hProcessHeap, 0, cbHashObject);
		pbHashData = (PBYTE)HeapAlloc(hProcessHeap, 0, cbHashDataLenght);
		if (NULL != pbHashObject && nullptr != pbHashData) {
			// create a hash
			if (NT_SUCCESS(status = BCryptCreateHash(m_hHashAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0))) {
				success = hashData(hHash, license, error, pbHashData, cbHashDataLenght) &&
						  signData(m_hTmpKey, pbHashData, cbHashDataLenght, error, signatureBuffer);
			} else {
				error = "error creating hash" + formatError(status);
			}
		} else {
			error = "**** memory allocation failed";
		}
	} else {
		error = "**** Error returned by BCryptGetProperty" + formatError(status);
	}

	if (hHash) {
		BCryptDestroyHash(hHash);
	}
	if (pbHashObject) {
		HeapFree(hProcessHeap, 0, pbHashObject);
	}
	if (pbHashData) {
		HeapFree(hProcessHeap, 0, pbHashData);
	}
	if (!success) {
		throw logic_error("Error signing data " + error);
	}
	return signatureBuffer;
}
} /* namespace license */
