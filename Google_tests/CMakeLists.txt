# 'Google_test' is the subproject name
project(Google_tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests
add_executable(Google_Tests_run code_request_test.cpp code_response_test.cpp data_request_test.cpp data_response_test.cpp clock_test.cpp)
target_link_libraries(Google_Tests_run gtest gtest_main)
