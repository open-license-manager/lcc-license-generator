cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

## configuration variables
## USE_OPENSSL: Enable/Disable OpenSSL support. If disabled, the generator will use Windows Crypto API on Windows and will fail to compile on Linux.
## STATIC_RUNTIME: Enable/Disable static linking of the executable. If enabled, the executable will be statically linked with all libraries, including OpenSSL and Boost. If disabled, the executable will be dynamically linked with all libraries.
## Boost_ROOT: Hint for a path to the Boost installation. If not set, CMake will try to find Boost in the system paths.
## OpenSSL_ROOT: Hint for a path to the Openssl installation. 

SET(CMAKE_DISABLE_SOURCE_CHANGES ON)
SET(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
SET(LICENSECC_SHORT_LICENSE "BSD Software License")

project (lccgen VERSION 2.1.0 LANGUAGES CXX)
set(PROJECT_DESCRIPTION "License generator for licensecc")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

IF(POLICY CMP0144)
	cmake_policy(SET CMP0144 NEW)
ENDIF(POLICY CMP0144)

include(utilities) #import module utilities.cmake

#Not sure about this. My system doesn't support binfmt misc
IF( ( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" ) AND CMAKE_CROSSCOMPILING AND NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR )
	SET(CMAKE_CROSSCOMPILING_EMULATOR "wine")	
ENDIF()

IF(NOT DEFINED STATIC_RUNTIME)
	SET(STATIC_RUNTIME FALSE CACHE BOOL "Compile a static executable or a dynamic linked one")
	MARK_AS_ADVANCED(STATIC_RUNTIME)
ENDIF(NOT DEFINED STATIC_RUNTIME)
message( STATUS "Static linking            : ${STATIC_RUNTIME}")


SET (OPENSSL_USE_STATIC_LIBS ${STATIC_RUNTIME})	

if(UNIX) #this is true for all the linux systems but not for cross compiling "linux->windows"
	
	IF(STATIC_RUNTIME)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
		unset(CMAKE_DL_LIBS)
		add_definitions(-DBOOST_TEST_STATIC_LINK)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
		set(BUILD_SHARED_LIBS OFF)
		#set(CMAKE_HAVE_LIBC_PTHREAD TRUE) #strange hack with cmake 4.3, why it's needed with static compile
	ENDIF(STATIC_RUNTIME)
	
	find_package(Threads)
	if(THREADS_HAVE_PTHREAD_ARG)
  		list(APPEND EXTERNAL_LIBS "-pthread")
	else(THREADS_HAVE_PTHREAD_ARG)
		if(CMAKE_THREAD_LIBS_INIT)
  			 list(APPEND EXTERNAL_LIBS "${CMAKE_THREAD_LIBS_INIT}")
		endif()
	endif(THREADS_HAVE_PTHREAD_ARG)
	
	IF(DEFINED USE_OPENSSL AND NOT USE_OPENSSL) #the user has explicitly disabled openssl, but we are in linux we need to enable it.
		message(FATAL_ERROR "USE_OPENSSL is set to OFF, but we are in linux. Please enable it.")
	ENDIF(DEFINED USE_OPENSSL AND NOT USE_OPENSSL)
	#openssl always required in linux
	find_package(OpenSSL REQUIRED COMPONENTS Crypto QUIET)
	add_definitions(-DHAS_OPENSSL)
	message( STATUS "OpenSSL libraries         :  ${OPENSSL_CRYPTO_LIBRARIES}")

	set(CTEST_MEMORYCHECK_TYPE "Valgrind")
	find_program( MEMORYCHECK_COMMAND valgrind )
	IF( MEMORYCHECK_COMMAND STREQUAL "MEMORYCHECK_COMMAND-NOTFOUND" )
		message( STATUS "valgrind                  : Not Found")
	ELSE()
		message( STATUS "valgrind                  : ${MEMORYCHECK_COMMAND}" )
	ENDIF()
	set( MEMORYCHECK_COMMAND_OPTIONS "--error-exitcode=1" )

	set(pkg_config_dest "lib/cmake/${PROJECT_NAME}")
else(UNIX) #windows and linux crosscompiling for windows target
	
	SET (OPENSSL_MSVC_STATIC_RT ${STATIC_RUNTIME})
	IF(DEFINED USE_OPENSSL)
		if(USE_OPENSSL)
			MESSAGE(VERBOSE "Openssl - generator       : forced by the user")
			find_package(OpenSSL REQUIRED COMPONENTS Crypto QUIET)
		else(USE_OPENSSL) #defined but evaluate
			MESSAGE(VERBOSE "Openssl - generator       : disabled by user.")
			set(OpenSSL_FOUND FALSE)
		endif(USE_OPENSSL)
	else(DEFINED USE_OPENSSL)
		SET (OPENSSL_MSVC_STATIC_RT TRUE)
		find_package(OpenSSL COMPONENTS Crypto QUIET)
	endif(DEFINED USE_OPENSSL)
	IF(OpenSSL_FOUND)
		add_definitions(-DHAS_OPENSSL)
		include_directories(${OPENSSL_INCLUDE_DIR})
		MESSAGE(STATUS "Openssl  (generator)      : ${OPENSSL_VERSION}")
		MESSAGE(STATUS "Openssl  (lib)            : ${OPENSSL_CRYPTO_LIBRARY}")
	ELSE(OpenSSL_FOUND) 
		MESSAGE(STATUS "Openssl - generator       : not found - using windows cryptography.")
	ENDIF(OpenSSL_FOUND)

	if(MSVC)
		include(target_arch)
		target_architecture( TARGET_ARCHITECTURE )
		message(STATUS "architecture detected     : ${TARGET_ARCHITECTURE}")
		
		#cmake > 3.15 handle the /MD flag more nicely than this
		if(STATIC_RUNTIME)
			string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
			string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
			string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
			string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
			SET(Boost_USE_STATIC_RUNTIME ON)
		endif(STATIC_RUNTIME)
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

SET (Boost_USE_STATIC_RUNTIME ${STATIC_RUNTIME}) #doesn't work well in Linux... 
SET	(Boost_USE_STATIC_LIBS ON )
find_package(Boost REQUIRED COMPONENTS date_time filesystem program_options unit_test_framework OPTIONAL_COMPONENTS system)

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
message( STATUS "Boost libs                : " ${Boost_LIBRARIES})

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

