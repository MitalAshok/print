cmake_minimum_required(VERSION 3.10)
project(print)

add_library(print INTERFACE)
target_sources(print INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include/print.h)
target_include_directories(print INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include/)

add_subdirectory(test)
