cmake_minimum_required(VERSION 3.22)

set(LICENSECC_SHORT_LICENSE "BSD Software License")
cmake_policy(SET CMP0048 NEW)

project(lccgen
	VERSION 2.1.0
	DESCRIPTION "License generator for licensecc"
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

find_package(OpenSSL COMPONENTS Crypto QUIET)
if (NOT OPENSSL_FOUND)
	message(FATAL_ERROR "Could not find OpenSSL package")
endif ()

configure_file (
	"src/build_properties.h.in" 
	"${CMAKE_BINARY_DIR}/include/build_properties.h"
)
include_directories("${CMAKE_BINARY_DIR}/include")

add_subdirectory("src/base_lib")
add_subdirectory("src/license_generator")
