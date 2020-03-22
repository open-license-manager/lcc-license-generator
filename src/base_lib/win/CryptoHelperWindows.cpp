/*
 * CryptoHelperWindows.cpp
 *
 *  Created on: Sep 14, 2014
 *
 */

#include <sstream>
#include <vector>
#include <string>

#include <windows.h>
#include <windef.h>
#include <bcrypt.h>
#include <ncrypt.h>
#include <wincrypt.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <boost/algorithm/string/predicate.hpp>

#include "../base64.h"
#include "CryptoHelperWindows.h"

//#pragma comment(lib, "bcrypt.lib")
//#pragma comment(lib, "crypt32.lib")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define RSA_KEY_BITLEN 1024

namespace license {
using namespace std;

/**************************
 * Module data sets and utility functions. (private)
 ****************************/
typedef struct _PKEY_BLOB {
	PUBLICKEYSTRUC publickeystruc;
	RSAPUBKEY rsapubkey;
	BYTE modulus[RSA_KEY_BITLEN / 8];
	BYTE prime1[RSA_KEY_BITLEN / 16];
	BYTE prime2[RSA_KEY_BITLEN / 16];
	BYTE exponent1[RSA_KEY_BITLEN / 16];
	BYTE exponent2[RSA_KEY_BITLEN / 16];
	BYTE coefficient[RSA_KEY_BITLEN / 16];
	BYTE privateExponent[RSA_KEY_BITLEN / 8];
} * PRSA_KEY_BLOB_1024, RSA_KEY_BLOB_1024;

static const string formatError(DWORD status) {
	std::ostringstream ss;
	ss << std::hex << status;
	vector<char> msgBuffer(256);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &msgBuffer[0],
				  sizeof(msgBuffer) - 1, nullptr);
	return string(&msgBuffer[0]) + ss.str();
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

		if (!NT_SUCCESS(status =
							BCryptExportKey(m_hTmpKey, nullptr, LEGACY_RSAPRIVATE_BLOB, nullptr, 0, &dwBlobLen, 0))) {
			throw logic_error(string("Error calculating size of private key ") +
							  to_string(static_cast<long long>(status)));
		}
		// Allocate memory for the pbKeyBlob.
		result.resize(dwBlobLen);

		// Do the actual exporting into the key BLOB.
		if (!NT_SUCCESS(status = BCryptExportKey(m_hTmpKey, NULL, LEGACY_RSAPRIVATE_BLOB, &result[0], dwBlobLen,
												 &dwBlobLen, 0))) {
			throw logic_error(string("Error exporting private key ") + to_string(static_cast<long long>(status)));
		}
		return result;
	}

	static void encodeLength(vector<uint8_t>& buffer, size_t object_lenght) {
		if (object_lenght <= 127) {
			buffer.push_back((uint8_t)object_lenght & 0x7F);
		} else {
			size_t sizeLen = ((size_t)(log(object_lenght) / log(256)) + 1);
			buffer.push_back((0x80 + (uint8_t)sizeLen) & 0xFF);

			for (int i = (int)sizeLen; i > 0; i--) {
				buffer.push_back((uint8_t)((object_lenght & 0xFF)));
				object_lenght >>= 8;
			}
		}
	}

	static void encode_sequence(vector<uint8_t>& encode) {
		size_t seqLen = encode.size();
		size_t encodeLen = 0;
		if (seqLen <= 127) {
			encodeLen = 2;  // tag + length
		} else {
			encodeLen = ((size_t)(log(seqLen) / log(256)) + 1) + 2;  // tag + size + extra bytes
		}
		encode.resize(seqLen + encodeLen);
		for (int i = (int)seqLen - 1; i >= 0; i--) {
			encode[i + encodeLen] = encode[i];
		}
		encode[0] = (0x30);  // sequence tag
		if (seqLen <= 127) {
			encode[1] = ((uint8_t)seqLen & 0x7F);
		} else {
			encode[1] = (0x80 + (uint8_t)(encodeLen - 2));

			for (int i = (int)(encodeLen - 2); i > 0; i--) {
				encode[i + 1] = ((uint8_t)((seqLen & 0xFF)));
				seqLen >>= 8;
			}
		}
	}

	static void encode_int(vector<uint8_t>& buffer, byte* intToWrite, size_t intSize) {
		// it is necessary to copy it reversed because it's big ending encoding.
		bool add_zero = ((intToWrite[intSize - 1] & 0x80) > 0);
		// skip initial zeroes
		int initial_zero = 0;
		for (int i = (int)(intSize - 1); i > 0 && (intToWrite[i] == 0); i--) {
			initial_zero++;
		}

		size_t finalSize = (add_zero ? (intSize + 1) : intSize) - initial_zero;
		buffer.push_back(0x02);  // int tag
		encodeLength(buffer, finalSize);

		if (add_zero) {
			buffer.push_back(0);
		}
		for (int i = (int)(intSize - 1 - initial_zero); i >= 0; i--) {
			uint8_t var = intToWrite[i];
			buffer.push_back(var);
		}
	}

	/** format of an encoded rsa private key
RSAPublicKey ::= SEQUENCE {
   modulus           INTEGER,  -- n
   publicExponent    INTEGER,  -- e
}
*/
	static vector<uint8_t> pkcs1_encode_rsa_blob_public(BYTE* pbKeyBlob) {
		PRSA_KEY_BLOB_1024 ptr = reinterpret_cast<PRSA_KEY_BLOB_1024>(pbKeyBlob);
		vector<uint8_t> result;
		result.reserve(sizeof(ptr->modulus) + sizeof(DWORD) + 128);
		encode_int(result, ptr->modulus, sizeof(ptr->modulus));
		encode_int(result, reinterpret_cast<byte*>(&ptr->rsapubkey.pubexp), sizeof(DWORD));
		encode_sequence(result);
		return result;
	}

	/** format of an encoded rsa private key
	RSAPrivateKey ::= SEQUENCE {
	   version           Version,
	   modulus           INTEGER,  -- n
	   publicExponent    INTEGER,  -- e
	   privateExponent   INTEGER,  -- d
	   prime1            INTEGER,  -- p
	   prime2            INTEGER,  -- q
	   exponent1         INTEGER,  -- d mod (p-1)
	   exponent2         INTEGER,  -- d mod (q-1)
	   coefficient       INTEGER,  -- (inverse of q) mod p
	   otherPrimeInfos   OtherPrimeInfos OPTIONAL
	}
	*/
	static vector<uint8_t> pkcs1_encode_rsa_blob_private(BYTE* pbKeyBlob) {
		PRSA_KEY_BLOB_1024 ptr = reinterpret_cast<PRSA_KEY_BLOB_1024>(pbKeyBlob);
		vector<uint8_t> result;
		result.reserve(sizeof(RSA_KEY_BLOB_1024) + 128);
		byte ver[] = {0x00};
		encode_int(result, ver, 1);
		encode_int(result, ptr->modulus, sizeof(ptr->modulus));
		encode_int(result, reinterpret_cast<byte*>(&ptr->rsapubkey.pubexp), sizeof(DWORD));
		encode_int(result, ptr->privateExponent, sizeof(ptr->privateExponent));
		encode_int(result, ptr->prime1, sizeof(ptr->prime1));
		encode_int(result, ptr->prime2, sizeof(ptr->prime2));
		encode_int(result, ptr->exponent1, sizeof(ptr->exponent1));
		encode_int(result, ptr->exponent2, sizeof(ptr->exponent2));
		encode_int(result, ptr->coefficient, sizeof(ptr->coefficient));
		encode_sequence(result);
		return result;
	}

	/*******************************
	Class methods (members)
	*************************/

	CryptoHelperWindows::CryptoHelperWindows() : m_hSignAlg(openSignatureProvider()), m_hHashAlg(openHashProvider()) {}

	/**
	 This method calls the BCryptGenerateKeyPair function to get a handle to an
	 exportable key-pair.
	 */
	void CryptoHelperWindows::generateKeyPair() {
		DWORD status;
		if (m_hTmpKey != nullptr) {
			BCryptDestroyKey(m_hTmpKey);
			m_hTmpKey = nullptr;
		}
		if (!NT_SUCCESS(status = BCryptGenerateKeyPair(m_hSignAlg, &m_hTmpKey, (ULONG)RSA_KEY_BITLEN, 0))) {
			const string err("error generating keypair" + formatError(status));
			throw logic_error(err);
		} else if (!NT_SUCCESS(status = BCryptFinalizeKeyPair(m_hTmpKey, 0))) {
			const string err("error finalizing keypair" + formatError(status));
			throw logic_error(err);
		}
	}

	const vector<unsigned char> CryptoHelperWindows::exportPublicKey() const {
		vector<uint8_t> pbKeyBlob = export_privateKey_blob(m_hTmpKey);
		return pkcs1_encode_rsa_blob_public(&pbKeyBlob[0]);
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
		vector<uint8_t> encoded = pkcs1_encode_rsa_blob_private(&pbKeyBlob[0]);
		ss << "-----BEGIN RSA PRIVATE KEY-----" << endl;
		ss << base64(&encoded[0], encoded.size(), 65);
		ss << "-----END RSA PRIVATE KEY-----" << endl;
		/*
		ofstream mystream;
		mystream.open("C:\\encoded.bin", fstream::binary | fstream::trunc);
		for (const auto& e : encoded) mystream << e;
		mystream.close();
		*/
		return ss.str();
	}

	void CryptoHelperWindows::loadPrivateKey(const std::string& privateKey) {
		DWORD dwBufferLen = 0, pkiLen = 0;
		LPBYTE pbBuffer = nullptr;
		PCRYPT_DER_BLOB pki = nullptr;
		string errors;

		if (!boost::starts_with(privateKey, "-----BEGIN RSA PRIVATE KEY-----")) {
			throw new logic_error("Private Key is not in the right format. It must be pkcs#1 encoded PEM.");
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
					DWORD status = BCryptImportKeyPair(m_hSignAlg, NULL, LEGACY_RSAPRIVATE_BLOB, &m_hTmpKey,
													   (PUCHAR)pki, pkiLen, 0);
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

	static bool hashData(BCRYPT_HASH_HANDLE& hHash, const string& data, string& error, PBYTE pbHash,
						 DWORD hashDataLenght) {
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
					DWORD finalSize;
					if (CryptBinaryToString(pbSignature, cbSignature, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
											nullptr, &finalSize)) {
						signatureBuffer.resize(
							finalSize -
							1);  // finalSize counts the \0 in the end, while string counts only the characters
						success =
							CryptBinaryToString(pbSignature, cbSignature, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
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
		BCRYPT_HASH_HANDLE hHash;
		string signatureBuffer;
		PBYTE pbHashObject = nullptr, pbHashData = nullptr;
		bool success = false;
		// calculate the size of the buffer to hold the hash object
		DWORD cbData = 0, cbHashObject = 0;
		// and the size to keep the hashed data
		DWORD cbHashDataLenght = 0;
		if (NT_SUCCESS(status = BCryptGetProperty(m_hHashAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD),
												  &cbData, 0)) &&
			NT_SUCCESS(status = BCryptGetProperty(m_hHashAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHashDataLenght,
												  sizeof(DWORD), &cbData, 0))) {
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
