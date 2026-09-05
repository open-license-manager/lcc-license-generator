cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

## Build options (set them with -D<OPTION>=<VALUE>):
## BOOST_ROOT                  Hint for a path to the Boost installation. If not set, CMake
##                             will try to find Boost in the system paths.
## BUILD_TESTING               Enable/disable building and running the tests. Only applies
##                             when this project is built standalone (not as a submodule).
##                             Default ON.
## OPENSSL_ROOT_DIR            Hint for a path to the OpenSSL installation. If not set, CMake
##                             will try to find OpenSSL in the system paths.
## STATIC_RUNTIME              Link statically to the C/C++ runtime libraries and to Boost
##                             and OpenSSL (when found). If disabled, the executable is
##                             dynamically linked to the system libraries. Default ON.
## USE_OPENSSL                 Enable/Disable OpenSSL support. On Linux it is always
##                             required and cannot be disabled. On Windows, when disabled,
##                             the Windows Crypto API (BCrypt) is used instead. If not
##                             specified on Windows, OpenSSL is searched anyway. Default OFF
##                             on Windows only.

SET(CMAKE_DISABLE_SOURCE_CHANGES ON) 
SET(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

IF(POLICY CMP0167)
	cmake_policy(SET CMP0167 OLD)
ENDIF(POLICY CMP0167)
IF(POLICY CMP0144)
	cmake_policy(SET CMP0144 NEW)
ENDIF(POLICY CMP0144)

project (lccgen VERSION 2.5.0 LANGUAGES CXX DESCRIPTION "License generator for Licensecc library")
SET(LICENSECC_SHORT_LICENSE "BSD Software License")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")


include(utilities) #import module utilities.cmake

#Not sure about this. My system doesn't support binfmt misc
IF( ( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" ) AND CMAKE_CROSSCOMPILING AND NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR )
	SET(CMAKE_CROSSCOMPILING_EMULATOR "wine")	
ENDIF()

option(STATIC_RUNTIME "Compile a static executable or a dynamic linked one" ON)
message( STATUS "Static   runtime          : ${STATIC_RUNTIME}")

#always prefer static linked version of the libraries.
SET (OPENSSL_USE_STATIC_LIBS ON)
SET (Boost_USE_STATIC_LIBS ON)

if(UNIX) #this is true for all the linux systems but not for cross compiling "linux->windows"
	
	IF(STATIC_RUNTIME)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
		unset(CMAKE_DL_LIBS)
		add_definitions(-DBOOST_TEST_STATIC_LINK)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
		set(BUILD_SHARED_LIBS OFF)
	ENDIF(STATIC_RUNTIME)
	find_package(Threads)
	if(THREADS_HAVE_PTHREAD_ARG)
  		list(APPEND EXTERNAL_LIBS "-pthread")
	else(THREADS_HAVE_PTHREAD_ARG)
		if(CMAKE_THREAD_LIBS_INIT)
  			 list(APPEND EXTERNAL_LIBS "${CMAKE_THREAD_LIBS_INIT}")
		endif()
	endif(THREADS_HAVE_PTHREAD_ARG)
	
	#openssl always required in linux
	IF(DEFINED USE_OPENSSL AND NOT USE_OPENSSL) #the user has explicitly disabled openssl, but we are in linux we need to enable it.
		message(FATAL_ERROR "USE_OPENSSL is set to OFF, but we are in linux. Please enable it.")
	ENDIF(DEFINED USE_OPENSSL AND NOT USE_OPENSSL)
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
		list(APPEND EXTERNAL_LIBS "bcrypt" "crypt32" "ws2_32" "iphlpapi")
	ENDIF(OpenSSL_FOUND)

	if(MSVC)
		#include(target_arch)
		#target_architecture( TARGET_ARCHITECTURE )
		#message(STATUS "architecture detected     : ${TARGET_ARCHITECTURE}")
		if(${STATIC_RUNTIME})
			SET(Boost_USE_STATIC_RUNTIME ON)
			unset(CMAKE_DL_LIBS)
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
		ELSE(${STATIC_RUNTIME})
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE INTERNAL "")
		endif(${STATIC_RUNTIME})
		add_definitions("/D _CRT_SECURE_NO_WARNINGS")
		add_compile_options("/EHsc")
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

#SET (Boost_NO_BOOST_CMAKE ON)
find_package(Boost REQUIRED COMPONENTS date_time filesystem program_options unit_test_framework OPTIONAL_COMPONENTS system)

#set below in case of dynamic linking in debug.
add_definitions(-DBOOST_ALL_NO_LIB) #Disable Boost Microsoft magic, all dependencies are handled by cmake
#add_definitions(-DBOOST_LIB_DIAGNOSTIC) #Check it is really disabled
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
message( STATUS "External libs             : " ${EXTERNAL_LIBS})
message( STATUS "Boost libs                : " ${Boost_LIBRARIES})
message( STATUS "Boost dirs                : " ${Boost_LIBRARY_DIRS})

#include (build directory)/include to find build_properties.h
include_directories(${CMAKE_BINARY_DIR}/include)
add_subdirectory("src/base_lib")
add_subdirectory("src/license_generator")

INCLUDE(CTest)
#do not build/run the tests if included as submodule. 
IF(BUILD_TESTING AND "${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
	MARK_AS_ADVANCED(BUILDNAME)
	add_subdirectory("test")
ENDIF(BUILD_TESTING AND "${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

# Configure CPack for different package formats
set(CPACK_PACKAGE_NAME "lcc-license-generator")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "open license manager Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LCC License Generator")
set(CPACK_PACKAGE_VENDOR "open license manager Team")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "open license manager Team")
#set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_OUTPUT_FILE_PREFIX "dist")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")

# Windows-specific settings
IF(WIN32)
	set(CPACK_GENERATOR "NSIS")
	set(CPACK_PACKAGING_INSTALL_PREFIX ".")
	#set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenLicenseManager")
	set(CPACK_NSIS_IGNORE_COMPONENTS_PAGE ON)
	set(CPACK_NSIS_MODIFY_PATH ON)
	set(CPACK_NSIS_MENU_LINKS "")
	#set(CPACK_MONOLITHIC_INSTALL ON)
	#set(CPACK_SET_DESTDIR "ON") 
	#set(CPACK_NSIS_MENU_LINKS "bin/lccgen.exe" "LCC License Generator")
	set(CPACK_NSIS_DISPLAY_NAME "LCC License Generator")
	set(CPACK_NSIS_HELP_LINK "https://github.com/your-repo/lcc-license-generator")
	set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/your-repo/lcc-license-generator")
ELSE(WIN32)
	if(NOT DEFINED CPACK_PACKAGING_INSTALL_PREFIX)
		set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
	endif(NOT DEFINED CPACK_PACKAGING_INSTALL_PREFIX)
	if(APPLE)
    	set(CPACK_GENERATOR "TGZ;productbuild")
    	set(CPACK_SYSTEM_NAME "Darwin-${CPACK_APPLE_ARCH}")
		set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
		set(CPACK_PRODUCTBUILD_DOMAINS_USER TRUE)
		set(CPACK_PRODUCTBUILD_DOMAINS_ROOT TRUE)
		set(CPACK_PRODUCTBUILD_DOMAINS_ANYWHERE FALSE)
		set(CPACK_PRODUCTBUILD_README "${PROJECT_SOURCE_DIR}/README.md")
    	set(CPACK_PRODUCTBUILD_IDENTIFIER "org.olm.lcc.license-generator")
	ELSE(APPLE)
		IF(UNIX)
			set(CPACK_GENERATOR "DEB;RPM;TGZ")
			set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
			set(CPACK_RPM_PACKAGE_LICENSE "BSD")
			set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
		ELSE(UNIX)
			message(WARNING "System ${CMAKE_SYSTEM_NAME} not supported, most probably package will fail")
		ENDIF(UNIX)
	ENDIF(APPLE)
ENDIF(WIN32)

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}_${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
# Source package settings
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-source")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\\\.git/"
    "/\\\\.gitignore"
    "/\\\\.DS_Store"
    "/\\\\.swp"
    "/\\\\.tmp"
	"/Testing/"
	"/out/"
    "/build/"
    "/dist/"
    "~$"
    "\\\\.(o|a)$"
    "\\\\.(so|dylib)$"
    "\\\\.(exe|dll)$"
)

include(CPack)
if(APPLE)
	cpack_add_component(lccgen_binaries)
ENDIF(APPLE)
