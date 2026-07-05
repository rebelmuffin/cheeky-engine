cmake_minimum_required(VERSION 3.13)

project(imgui LANGUAGES CXX VERSION 1.92.9)

add_library(imgui STATIC)

find_package(Vulkan REQUIRED)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

target_include_directories(imgui PUBLIC .)

target_sources(imgui
        PRIVATE
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        backends/imgui_impl_sdl3.cpp
        backends/imgui_impl_vulkan.cpp
)

add_definitions(-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES)