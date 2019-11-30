link_directories ( ${Boost_LIBRARY_DIR} )

ADD_LIBRARY(
    license_generator_lib STATIC
#    license-generator.cpp
    command_line-parser.cpp
    license.cpp
    project.cpp
    ../ini/ConvertUTF.cpp
)

target_link_libraries(
     license_generator_lib
     lcc_base
     ${Boost_LIBRARIES}	 
     ${EXTERNAL_LIBS}
)

option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(license_generator_lib INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(license_generator_lib INTERFACE --coverage)
    target_link_libraries(license_generator_lib PUBLIC gcov)
  else()
    target_link_libraries(license_generator_lib gcov)
  endif()
endif(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

add_executable(
 lcc
 open-license-main.cpp
)
add_executable(license_generator::lcc ALIAS lcc)

target_link_libraries(
    lcc PRIVATE
    license_generator_lib
)

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
