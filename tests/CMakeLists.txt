cmake_minimum_required(VERSION 3.10.0)
project(tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/catch)

add_executable(tests
  catch_main.cpp
  test_dolist.cpp
  test_vector.cpp
  test_stack.cpp
  test_queue.cpp
  test_rtree.cpp
)
