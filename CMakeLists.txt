cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

SET(CMAKE_DISABLE_SOURCE_CHANGES ON)
SET(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
SET(LICENSECC_SHORT_LICENSE "BSD Software License")

project (olm-license-generator CXX)

#SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(utilities) #import module utilities.cmake
if(CMAKE_BUILD_TYPE)
	string(TOLOWER ${CMAKE_BUILD_TYPE} _CMAKE_BUILD_TYPE)
	EVAL_CONDITION(RELEASE_BUILD ${_CMAKE_BUILD_TYPE} STREQUAL "release")
else(CMAKE_BUILD_TYPE)
	SET(RELEASE_BUILD 0)
endif(CMAKE_BUILD_TYPE)

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
else(MSVC)
    if(MINGW)
        list(APPEND EXTERNAL_LIBS "-lcrypt32 -lws2_32 -liphlpapi")
        SET(CMAKE_EXE_LINKER_FLAGS "-static")
    endif(MINGW)
endif(MSVC)

if(UNIX) #this is true for all the linux systems but not for cross compiling "linux->windows"
	#find a static version of openssl crypto library
	SET ( OPENSSL_USE_STATIC_LIBS ON )
	find_package(OpenSSL REQUIRED COMPONENTS Crypto)
	include_directories(${OPENSSL_INCLUDE_DIR})
    list(APPEND EXTERNAL_LIBS ${OPENSSL_CRYPTO_LIBRARY})
	MESSAGE(STATUS "Found openssl version ${OPENSSL_VERSION} ")
	#Zlib required when openssl version < 1.0.1f
	SET ( ZLIB_USE_STATIC_LIBS ON )
	find_package(ZLIB REQUIRED)
	list(APPEND EXTERNAL_LIBS ${ZLIB_LIBRARIES})
	MESSAGE(STATUS "Found zlib version ${ZLIB_VERSION} ")
	if(NOT MINGW)
 		list(APPEND EXTERNAL_LIBS "-ldl") 
	endif(NOT MINGW)
	find_program( MEMORYCHECK_COMMAND valgrind )
	set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" )
	if(USE_DBUS_IDENTIFIER)
	    FIND_PACKAGE(Dbus REQUIRED)
	    add_definitions(-DUSE_DBUS)
	    include_directories(${DBUS_INCLUDE_DIR})
	    include_directories(${DBUS_ARCH_INCLUDE_DIR})
	    list(APPEND EXTERNAL_LIBS ${DBUS_LIBRARIES})
	endif(USE_DBUS_IDENTIFIER)    
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
find_package(Boost REQUIRED COMPONENTS date_time unit_test_framework program_options system filesystem)
#set below in case of dynamic linking in debug.
#set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:BOOST_TEST_DYN_LINK>)
add_definitions(-DBOOST_ALL_NO_LIB) #Disable Boost Microsoft magic, all dependencies are handled by cmake
add_definitions(-DBOOST_LIB_DIAGNOSTIC) #Check it is really disabled

configure_file (
	"src/build_properties.h.in" 
	"${CMAKE_BINARY_DIR}/include/build_properties.h"
)

message( STATUS "CXX compiler      : " ${CMAKE_CXX_COMPILER})
message( STATUS "CXX compiler flags: " ${CMAKE_CXX_FLAGS})
message( STATUS "CXX compiler flags debug : " ${CMAKE_CXX_FLAGS_DEBUG})
message( STATUS "CXX compiler flags release: " ${CMAKE_CXX_FLAGS_RELEASE})
if(CMAKE_BUILD_TYPE)
  message( STATUS "Build type        : " ${CMAKE_BUILD_TYPE})
endif(CMAKE_BUILD_TYPE)
message( STATUS "Install prefix    : " ${CMAKE_INSTALL_PREFIX})

#include (build directory)/include to find build_properties.h
include_directories(${CMAKE_BINARY_DIR}/include)
add_subdirectory("src/base_lib")
add_subdirectory("src/ini")
add_subdirectory("src/license_generator")

INCLUDE(CTest)
IF(BUILD_TESTING)
  SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
  MARK_AS_ADVANCED(BUILDNAME)
  add_subdirectory("test")
ENDIF(BUILD_TESTING)
