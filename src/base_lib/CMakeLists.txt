if(UNIX OR OPENSSL_FOUND)

	ADD_LIBRARY(
	    lcc_base STATIC
	    base64.cpp
	    crypto_helper.cpp
	    openssl/crypto_helper_ssl.cpp
	)
	
	if (CMAKE_VERSION VERSION_GREATER 3.5)
		target_link_libraries(lcc_base
		     ${Boost_LIBRARIES} OpenSSL::Crypto ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
	ELSE(CMAKE_VERSION VERSION_GREATER 3.5)
		target_link_libraries(lcc_base
			     ${Boost_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY} ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_VERSION VERSION_GREATER 3.5)
	
else(UNIX OR OPENSSL_FOUND) #windows and zlib not found
	ADD_LIBRARY(
	    lcc_base STATIC
	    base64.cpp
	    crypto_helper.cpp
	    win/CryptoHelperWindows.cpp
	)
	target_link_libraries(
	     lcc_base
	     ${Boost_LIBRARIES}
	     ${EXTERNAL_LIBS}
	)
ENDIF(UNIX OR OPENSSL_FOUND)

IF(BUILD_TESTING)
	option(CODE_COVERAGE "Enable coverage reporting" OFF)
	if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	  target_compile_options(lcc_base INTERFACE
	    -O0        # no optimization
	    -g         # generate debug info
	    --coverage # sets all required flags
	  )
	  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
	    target_link_options(lcc_base INTERFACE --coverage)
	    target_link_libraries(lcc_base PUBLIC gcov)
	  else()
	    target_link_libraries(lcc_base gcov)
	  endif()
	endif(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
ENDIF(BUILD_TESTING)
