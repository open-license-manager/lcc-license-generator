link_directories ( ${Boost_LIBRARY_DIR} )

ADD_LIBRARY(license_generator_lib STATIC command_line-parser.cpp license.cpp project.cpp ../ini/ConvertUTF.cpp)
target_link_libraries(license_generator_lib lcc_base ${Boost_LIBRARIES} ${EXTERNAL_LIBS})

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
