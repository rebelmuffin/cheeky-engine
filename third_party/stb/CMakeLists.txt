cmake_minimum_required(VERSION 3.16)
project(stb LANGUAGES CXX)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")