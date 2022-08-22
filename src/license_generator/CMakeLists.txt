#link_directories ( ${Boost_LIBRARY_DIR} )

ADD_LIBRARY(license_generator_lib 
	STATIC command_line-parser.cpp license.cpp project.cpp ../ini/ConvertUTF.cpp
	$<TARGET_OBJECTS:lcc_base> )

target_include_directories(license_generator_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(UNIX OR OPENSSL_FOUND)
	target_link_libraries(license_generator_lib PUBLIC
			     ${Boost_LIBRARIES} OpenSSL::Crypto ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
else(UNIX OR OPENSSL_FOUND) #windows and zlib not found
	target_link_libraries(license_generator_lib PUBLIC ${Boost_LIBRARIES} ${EXTERNAL_LIBS})
ENDIF(UNIX OR OPENSSL_FOUND)

if(CODE_COVERAGE AND UNIX)
	  target_compile_options(license_generator_lib PUBLIC -O0 -g --coverage)
	  target_link_libraries(license_generator_lib PUBLIC gcov)
	  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
	    target_link_options(license_generator_lib PUBLIC --coverage)
	  endif()
endif(CODE_COVERAGE AND UNIX)

add_executable(lccgen open-license-main.cpp)
add_executable(license_generator::lccgen ALIAS lccgen)
target_link_libraries(lccgen PRIVATE license_generator_lib)

#install instructions
install(TARGETS lccgen EXPORT license_generator RUNTIME DESTINATION bin)

install(EXPORT license_generator NAMESPACE license_generator:: DESTINATION ${pkg_config_dest})
install(FILES ../../cmake/lccgen-config.cmake DESTINATION ${pkg_config_dest})

#if we're building as standalone export the current build directory for easy retrieval 
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
	message(STATUS "Exporting current build directory")
	export(TARGETS lccgen NAMESPACE license_generator:: FILE ${CMAKE_BINARY_DIR}/lccgen-config.cmake)
	export(PACKAGE lccgen)
endif()



