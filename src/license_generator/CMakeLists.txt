link_directories ( ${Boost_LIBRARY_DIR} )

ADD_LIBRARY(license_generator_lib 
	STATIC command_line-parser.cpp license.cpp project.cpp ../ini/ConvertUTF.cpp
	$<TARGET_OBJECTS:lcc_base> )

if(UNIX OR OPENSSL_FOUND)
	target_link_libraries(license_generator_lib 
			     ${Boost_LIBRARIES} OpenSSL::Crypto ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
else(UNIX OR OPENSSL_FOUND) #windows and zlib not found
	target_link_libraries(license_generator_lib  ${Boost_LIBRARIES} ${EXTERNAL_LIBS})
ENDIF(UNIX OR OPENSSL_FOUND)

if(CODE_COVERAGE AND UNIX)
	  target_compile_options(license_generator_lib PUBLIC -O0 -g --coverage)
	  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
	    target_link_options(license_generator_lib PUBLIC --coverage)
	    target_link_libraries(license_generator_lib PUBLIC gcov)
	  else()
	    target_link_libraries(license_generator_lib gcov)
	  endif()
endif(CODE_COVERAGE AND UNIX)

add_executable(lcc open-license-main.cpp)
add_executable(license_generator::lcc ALIAS lcc)
target_link_libraries(lcc PRIVATE license_generator_lib)

#install instructions
install(TARGETS lcc EXPORT license_generator RUNTIME DESTINATION bin)

set(main_lib_dest "lib/lcc")
install(EXPORT license_generator NAMESPACE license_generator:: DESTINATION ${main_lib_dest})
install(FILES ../../cmake/lcc-config.cmake DESTINATION ${main_lib_dest})

#if we're building as standalone export the current build directory for easy retrieval 
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
	message(STATUS "Exporting current build directory")
	export(TARGETS lcc NAMESPACE license_generator:: FILE ${CMAKE_BINARY_DIR}/lcc-config.cmake)
	export(PACKAGE lcc)
endif()



