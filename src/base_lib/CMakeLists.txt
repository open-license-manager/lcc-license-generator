if(WIN32)
	ADD_LIBRARY(
	    olm_base STATIC
	    base64.cpp
	    crypto_helper.cpp
	    win/CryptoHelperWindows.cpp
	)
	target_link_libraries(
	     olm_base
	     ${Boost_LIBRARIES}
	     ${EXTERNAL_LIBS}
	)

else(WIN32)
	ADD_LIBRARY(
	    olm_base STATIC
	    base64.cpp
	    crypto_helper.cpp
	    linux/CryptoHelperLinux.cpp
	)
	
	#patch for old cmake in Centos 7
	if (CMAKE_VERSION VERSION_GREATER 3.5)
		target_link_libraries(olm_base
		     ${Boost_LIBRARIES} OpenSSL::Crypto ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
	ELSE(CMAKE_VERSION VERSION_GREATER 3.5)
		target_link_libraries(olm_base
			     ${Boost_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY} ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
	ENDIF(CMAKE_VERSION VERSION_GREATER 3.5)
ENDIF(WIN32)


#install(TARGETS olm_base ARCHIVE DESTINATION lib/${PROJECT_NAME})
