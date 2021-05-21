cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

SET(CMAKE_DISABLE_SOURCE_CHANGES ON)
SET(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
SET(LICENSECC_SHORT_LICENSE "BSD Software License")

cmake_policy(SET CMP0048 NEW)
project (lccgen
	VERSION 2.0.0
	DESCRIPTION "License generator for licensecc" 
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(utilities) #import module utilities.cmake

#Not sure about this. My system doesn't support binfmt misc
IF( ( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" ) AND CMAKE_CROSSCOMPILING AND NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR )
	SET(CMAKE_CROSSCOMPILING_EMULATOR "wine")	
ENDIF()

SET ( OPENSSL_USE_STATIC_LIBS ON )
find_package(OpenSSL COMPONENTS Crypto QUIET)
IF(OPENSSL_FOUND)
	add_definitions(-DHAS_OPENSSL)
	include_directories(${OPENSSL_INCLUDE_DIR})
	MESSAGE(STATUS "Found openssl version ${OPENSSL_VERSION} - generator")
ENDIF(OPENSSL_FOUND)

if(UNIX) #this is true for all the linux systems but not for cross compiling "linux->windows"
	#openssl required for linux
	IF(NOT OPENSSL_FOUND)
		MESSAGE(SEND_ERROR "Openssl not found - generator - required in Linux")
	ENDIF(NOT OPENSSL_FOUND)
	IF(${STATIC_RUNTIME})
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--whole-archive -static-libgcc -static-libstdc++ -static")
		unset(CMAKE_DL_LIBS)
	ENDIF(${STATIC_RUNTIME})
	
    find_package(Threads)
	if(THREADS_HAVE_PTHREAD_ARG)
  		list(APPEND EXTERNAL_LIBS "-pthread")
	else(THREADS_HAVE_PTHREAD_ARG)
		if(CMAKE_THREAD_LIBS_INIT)
  			 list(APPEND EXTERNAL_LIBS "${CMAKE_THREAD_LIBS_INIT}")
		endif()
	endif(THREADS_HAVE_PTHREAD_ARG)
	
	#Zlib required when openssl version < 1.0.2k (centos 7)
	IF(OPENSSL_VERSION VERSION_LESS_EQUAL 1.0.2)
		SET ( ZLIB_USE_STATIC_LIBS ON ) 
		find_package(ZLIB REQUIRED)
		list(APPEND EXTERNAL_LIBS ${ZLIB_LIBRARIES})
		MESSAGE(STATUS "Found zlib version ${ZLIB_VERSION_STRING} - generator")
	ENDIF(OPENSSL_VERSION VERSION_LESS_EQUAL 1.0.2)

	find_program( MEMORYCHECK_COMMAND valgrind )
	set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" ) 
	set(pkg_config_dest "lib/cmake/${PROJECT_NAME}")
else(UNIX) #windows and linux crosscompiling for windows target
	IF(NOT OPENSSL_FOUND)
		MESSAGE(STATUS "Openssl not found - generator - configuring libary without it.")
	ENDIF(NOT OPENSSL_FOUND)
	if(MSVC)
		include(target_arch)
		target_architecture( TARGET_ARCHITECTURE )
		message(STATUS "architecture detected: ${TARGET_ARCHITECTURE}")
		
		#Boost > 3.15 handle the /MD flag more nicely than this
		if(${STATIC_RUNTIME})
			string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
			string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
			string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
			string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
			SET(Boost_USE_STATIC_RUNTIME ON)
		endif(${STATIC_RUNTIME})
		add_definitions("/D _CRT_SECURE_NO_WARNINGS")
		list(APPEND EXTERNAL_LIBS "bcrypt" "crypt32" "ws2_32" "iphlpapi")
	else(MSVC)
	    if(MINGW)
	        list(APPEND EXTERNAL_LIBS "-lcrypt32 -lbcrypt -lws2_32 -liphlpapi")
	        SET(CMAKE_EXE_LINKER_FLAGS "-static")
	    endif(MINGW)
	endif(MSVC)
	set(pkg_config_dest "cmake/${PROJECT_NAME}")
endif(UNIX)

#bug in cmake win32 - can't find boost compiled with mingw 
if(WIN32 AND "x${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "x")
    message(WARNING "WIN32 compiler does not specify CMAKE_CXX_COMPILER_ARCHITECTURE_ID -- filling in manually")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID "x64")
    else()
        set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID "x86")
    endif()
    message(STATUS "Compiler architecture: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
endif()

SET( Boost_USE_STATIC_LIBS ON )
find_package(Boost REQUIRED COMPONENTS date_time filesystem program_options system unit_test_framework)

#set below in case of dynamic linking in debug.
#set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:BOOST_TEST_DYN_LINK>)
add_definitions(-DBOOST_ALL_NO_LIB) #Disable Boost Microsoft magic, all dependencies are handled by cmake
add_definitions(-DBOOST_LIB_DIAGNOSTIC) #Check it is really disabled
include_directories(${Boost_INCLUDE_DIRS}) 

configure_file (
	"src/build_properties.h.in" 
	"${CMAKE_BINARY_DIR}/include/build_properties.h"
)

message( STATUS "CXX compiler              : " ${CMAKE_CXX_COMPILER})
message( STATUS "CXX compiler flags        : " ${CMAKE_CXX_FLAGS})
message( STATUS "CXX compiler flags debug  : " ${CMAKE_CXX_FLAGS_DEBUG})
message( STATUS "CXX compiler flags release: " ${CMAKE_CXX_FLAGS_RELEASE})
if(CMAKE_BUILD_TYPE)
  message( STATUS "Build type                : " ${CMAKE_BUILD_TYPE})
endif(CMAKE_BUILD_TYPE)
message( STATUS "Install prefix            : " ${CMAKE_INSTALL_PREFIX})
#include (build directory)/include to find build_properties.h
message( STATUS "External libs             : " ${EXTERNAL_LIBS})
include_directories(${CMAKE_BINARY_DIR}/include)
add_subdirectory("src/base_lib")
add_subdirectory("src/license_generator")

INCLUDE(CTest)
IF(BUILD_TESTING)
    SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
	MARK_AS_ADVANCED(BUILDNAME)
	add_subdirectory("test")
ENDIF(BUILD_TESTING)

set(CPACK_GENERATOR "DEB;TBZ2;RPM")
set(CPACK_PACKAGE_NAME "lcc-generator")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "open license manager Team")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_OUTPUT_FILE_PREFIX dist)
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
include(CPack)

