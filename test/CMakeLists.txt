add_executable(test_license license_test.cpp)
target_link_libraries(test_license license_generator_lib ${Boost_LIBRARIES})
ADD_TEST(NAME test_license COMMAND test_license WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(test_command-line command-line_test.cpp)
target_link_libraries(test_command-line license_generator_lib ${Boost_LIBRARIES})
ADD_TEST(NAME test_command-line COMMAND test_command-line WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(test_project project_test.cpp)
target_link_libraries(test_project license_generator_lib ${Boost_LIBRARIES})
ADD_TEST(NAME test_project COMMAND test_project WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(test_cryptohelper cryptohelper_test.cpp)
target_link_libraries(test_cryptohelper license_generator_lib ${Boost_LIBRARIES})
ADD_TEST(NAME test_cryptohelper COMMAND test_cryptohelper WORKING_DIRECTORY ${CMAKE_BINARY_DIR})