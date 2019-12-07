#ifndef CRYPTPHELPER_H_
#define CRYPTPHELPER_H_

#include <memory>
#include <cstddef>
#include <string>
#include <vector>

namespace license {

/**
 * Helper class definition to generate and export Public/Private keys
 * for Asymmetric encryption.
 *
 * <p>Since this part relies heavily on operating system libraries this class
 * provides a common facade to the cryptographic functions. The two implementing
 * subclasses are chosen in the factory method #getInstance(). This is to avoid
 * to clutter the code with many "ifdef". (extreme performance is not an issue here)</p>
 *
 * <p>Private keys are 1024 bits openssl format. Public keys are in binary format (for security reasons).
 * Signatures are in base64</p>
 */

class CryptoHelper {
protected:
	inline CryptoHelper() {}

public:
	virtual void generateKeyPair() = 0;
	const virtual std::string exportPrivateKey() const = 0;
	const virtual std::vector<unsigned char> exportPublicKey() const = 0;

	/**
	 * Load the private key from a file.
	 * @param privateKey_file_name
	 * name of the file where the private key is stored.
	 */
	virtual void loadPrivateKey_file(const std::string &privateKey_file_name);

	/**
	 * Load the private key from a string.
	 * @param privateKey
	 * string containing the private key representation
	 */
	virtual void loadPrivateKey(const std::string &privateKey) = 0;
	/**
	 * signature algorithm SHA256withRSA
	 * @param license
	 * @return
	 */
	const virtual std::string signString(const std::string &license) const = 0;
	static std::unique_ptr<CryptoHelper> getInstance();
	virtual ~CryptoHelper() {}
};
}  // namespace license
#endif
