include_directories(${Boost_INCLUDE_DIR})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories ( ${Boost_LIBRARY_DIR} )

ADD_LIBRARY(
    license_generator_lib STATIC
    license-generator.cpp
    command_line-parser.cpp
    project.cpp
)

target_link_libraries(
     license_generator_lib
     olm_base
     ini
	 $<$<CONFIG:Debug>:${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}>
     $<$<NOT:$<CONFIG:Debug>>:${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE}>	 
	 $<$<CONFIG:Debug>:${Boost_SYSTEM_LIBRARY_DEBUG}>
     $<$<NOT:$<CONFIG:Debug>>:${Boost_SYSTEM_LIBRARY_RELEASE}>	
     $<$<CONFIG:Debug>:${Boost_FILESYSTEM_LIBRARY_DEBUG}>
     $<$<NOT:$<CONFIG:Debug>>:${Boost_FILESYSTEM_LIBRARY_DEBUG}>	 
     ${EXTERNAL_LIBS}
)

add_executable(
 olm
 open-license-main.cpp
)

target_link_libraries(
    olm
    license_generator_lib
)

install(TARGETS olm EXPORT license_generator 
	RUNTIME DESTINATION bin)

#set_property(TARGET olm PROPERTY VERSION ${OLM_VERSION_STRING})
install(EXPORT license_generator 
	NAMESPACE license_generator:: 
	DESTINATION lib/olm)

set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(TARGETS olm NAMESPACE license_generator:: FILE olm-config.cmake)
export(PACKAGE olm)
#export(TARGETS olm NAMESPACE license_manager:: FILE OlmTargets.cmake)
#install(EXPORT olm-targets DESTINATION lib/olm/olm-targets.cmake)

#install(TARGETS license_generator_lib ARCHIVE DESTINATION lib/${PROJECT_NAME})

#add_custom_command(
#     OUTPUT ${CMAKE_BINARY_DIR}/test_project/src/CMakeLists.txt
#            ${CMAKE_BINARY_DIR}/test_project/private-key.h
#     COMMAND olm project initialize -n test_project --source ${CMAKE_SOURCE_DIR}/src/library --project ${CMAKE_BINARY_DIR}
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#     DEPENDS olm
#     COMMENT "Initializing library build for tests")

#add_custom_target(private_key DEPENDS "${CMAKE_BINARY_DIR}/test_project/private-key.h")
#add_custom_target(lib_configured DEPENDS "${CMAKE_BINARY_DIR}/test_project/private-key.h")
#add_custom_command(TARGET olm POST_BUILD COMMAND olm project initialize -n test_project -s -p
#                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#                   COMMENT "Initializing library build for tests")