cmake_minimum_required (VERSION 3.1)
project (test_main CXX)

set (CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES 
  fir_regression
  iir_regression
)
include_directories("../src")
add_executable(test_main main.cc)
