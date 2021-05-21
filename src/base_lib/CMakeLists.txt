if(UNIX OR OPENSSL_FOUND)

	ADD_LIBRARY(
	    lcc_base OBJECT
	    base64.cpp
	    crypto_helper.cpp
	    openssl/crypto_helper_ssl.cpp
	)
	
else(UNIX OR OPENSSL_FOUND) #windows and zlib not found
	ADD_LIBRARY(
	    lcc_base OBJECT
	    base64.cpp
	    crypto_helper.cpp
	    win/CryptoHelperWindows.cpp
	)
ENDIF(UNIX OR OPENSSL_FOUND)

if(CODE_COVERAGE AND UNIX)
	  MESSAGE(STATUS "Enabling code coverage")
	  target_compile_options(lcc_base PUBLIC
	    -O0        # no optimization
	    -g         # generate debug info
	    --coverage # sets all required flags
	  )
endif(CODE_COVERAGE AND UNIX)
