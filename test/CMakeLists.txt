
add_executable(
 license_generator_test
 license-generator_test.cpp
)

target_link_libraries(
 license_generator_test
 license_generator_lib
 ${Boost_LIBRARIES}
)

IF( ( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") AND CMAKE_CROSSCOMPILING)
	ADD_TEST(NAME license_generator_test COMMAND wine ${CMAKE_CURRENT_BINARY_DIR}/license_generator_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
ELSE()
	ADD_TEST(NAME license_generator_test COMMAND license_generator_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
ENDIF()

add_executable(
 test_product
 product_test.cpp
)

target_link_libraries(
 test_product
 license_generator_lib
 ${Boost_LIBRARIES}
)

ADD_TEST(NAME test_product COMMAND test_product WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(
 test_cryptohelper
 cryptohelper_test.cpp
)

target_link_libraries(
 test_cryptohelper
 license_generator_lib
 ${Boost_LIBRARIES}
)

ADD_TEST(NAME test_cryptohelper COMMAND test_cryptohelper WORKING_DIRECTORY ${CMAKE_BINARY_DIR})