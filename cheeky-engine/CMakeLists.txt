cmake_minimum_required(VERSION 3.24)

# Add source to this project's executable.
file(GLOB_RECURSE ENGINE_SRC "src/Private/*.cpp")

add_executable(cheeky ${ENGINE_SRC}
        src/Private/Renderer/Utility/Camera.cpp)

set_property(TARGET cheeky PROPERTY
        DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        CXX_STANDARD 20
        COMPILE_WARNING_AS_ERROR ON
)

target_compile_options(cheeky PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

find_package(Vulkan REQUIRED)
message("Found Vulkan Version: ${Vulkan_VERSION}")
message("Found Vulkan Libraries: ${Vulkan_INCLUDE_DIRS}")

target_include_directories(cheeky PUBLIC "src/Public")
target_link_libraries(cheeky
        PUBLIC
        Vulkan::Vulkan
        SDL3::SDL3
        glm::glm-header-only
        VulkanMemoryAllocator
        volk
        vk-bootstrap
        imguizmo::imguizmo
        imgui
        fastgltf::fastgltf
        stb
        tbb
)

# move SDL3.dll to the correct place
add_custom_command(TARGET cheeky POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:cheeky>
)

# for glm vector rotation
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)