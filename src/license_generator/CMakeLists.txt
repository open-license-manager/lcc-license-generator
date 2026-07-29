#link_directories ( ${Boost_LIBRARY_DIR} )

ADD_LIBRARY(license_generator_lib 
	STATIC command_line-parser.cpp license.cpp project.cpp ../ini/ConvertUTF.cpp
	$<TARGET_OBJECTS:lcc_base> )

target_include_directories(license_generator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX)
	set_target_properties(license_generator_lib PROPERTIES POSITION_INDEPENDENT_CODE ${STATIC_RUNTIME})
	target_link_libraries(license_generator_lib PUBLIC
			     ${Boost_LIBRARIES} OpenSSL::Crypto ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
else(UNIX)
  	if(OpenSSL_FOUND) #windows and openssl found
		target_link_libraries(license_generator_lib PUBLIC ${Boost_LIBRARIES} 
			OpenSSL::Crypto ${EXTERNAL_LIBS})
	else(OpenSSL_FOUND)
		target_link_libraries(license_generator_lib PUBLIC
			     ${Boost_LIBRARIES} ${EXTERNAL_LIBS} ${CMAKE_DL_LIBS})
	endif(OpenSSL_FOUND)
ENDIF(UNIX)
get_target_property(FULL_LIBS license_generator_lib LINK_LIBRARIES)
message( VERBOSE "Link libs for license_gen : " ${FULL_LIBS})

message( VERBOSE "Code coverage report      : " ${CODE_COVERAGE})
if(CODE_COVERAGE AND UNIX)
	  target_compile_options(license_generator_lib PUBLIC -O0 -g --coverage -fprofile-arcs -ftest-coverage)
	  target_link_libraries(license_generator_lib PUBLIC gcov)
	  target_link_options(license_generator_lib PUBLIC --coverage -fprofile-arcs -ftest-coverage)
endif(CODE_COVERAGE AND UNIX)

add_executable(lccgen open-license-main.cpp)
add_executable(license_generator::lccgen ALIAS lccgen)
target_link_libraries(lccgen PRIVATE license_generator_lib)
set_target_properties(lccgen PROPERTIES VERSION ${PROJECT_VERSION})

#install instructions
install(TARGETS lccgen EXPORT license_generator 
			RUNTIME PERMISSIONS WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ OWNER_EXECUTE OWNER_WRITE OWNER_READ
			COMPONENT lccgen_binaries)

install(EXPORT license_generator NAMESPACE license_generator:: DESTINATION ${pkg_config_dest} COMPONENT lccgen_binaries)
##double check this
#install(FILES ../../cmake/lccgen-config.cmake DESTINATION ${pkg_config_dest} COMPONENT lccgen-binaries)

#if we're building as standalone export the current build directory for easy retrieval 
#if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
#	message(STATUS "Exporting current build directory")
#	export(TARGETS lccgen NAMESPACE license_generator:: FILE ${CMAKE_BINARY_DIR}/lccgen-config.cmake)
#	export(PACKAGE lccgen)
#endif()



