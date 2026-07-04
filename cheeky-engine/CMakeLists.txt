cmake_minimum_required(VERSION 3.13)

# Add source to this project's executable.
file(GLOB_RECURSE ENGINE_SRC "src/Private/*.cpp")

add_executable(cheeky ${ENGINE_SRC})

set_property(TARGET cheeky PROPERTY
        DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET cheeky PROPERTY CXX_STANDARD 20)
endif ()

target_include_directories(cheeky PUBLIC "src/public")
target_link_libraries(cheeky
        PUBLIC
        SDL3::SDL3
        glm::glm-header-only
        VulkanMemoryAllocator
        volk
        vk-bootstrap
        imguizmo::imguizmo
        imgui
        fastgltf::fastgltf
        stb
)

# move SDL3.dll to the correct place
add_custom_command(TARGET cheeky POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:cheeky>
)

# for glm vector rotation
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)