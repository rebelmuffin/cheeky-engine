cmake_minimum_required(VERSION 3.13)

project(imgui LANGUAGES CXX VERSION 1.92.9)

add_library(imgui
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        backends/imgui_impl_sdl3.cpp
        backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC .)

add_definitions(-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES)