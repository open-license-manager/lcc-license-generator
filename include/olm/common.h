#ifndef OLM_COMMON_H_
#define OLM_COMMON_H_

#include <string>

namespace license {

class FullLicenseInfo {
public:
	std::string source;
	std::string product;
	std::string license_signature;
	int license_version;
	std::string from_date;
	std::string to_date;
	bool has_expiry;
	unsigned int from_sw_version;
	unsigned int to_sw_version;
	bool has_versions;
	std::string client_signature;
	bool has_client_sig;
	std::string extra_data;

	static const char *UNUSED_TIME;
	static const unsigned int UNUSED_SOFTWARE_VERSION = 0;

	FullLicenseInfo(const std::string& source, const std::string& product,
			const std::string& license_signature, int licenseVersion,
			std::string from_date = UNUSED_TIME,
			std::string to_date = UNUSED_TIME, //
			const std::string& client_signature = "", //
			unsigned int from_sw_version = UNUSED_SOFTWARE_VERSION,
			unsigned int to_sw_version = UNUSED_SOFTWARE_VERSION,
			const std::string& extra_data = "");
	//std::string printForSign() const;
	//void printAsIni(std::ostream & a_ostream) const;
	//time_t expires_on() const;
	//time_t valid_from() const;
};

}

#endif
