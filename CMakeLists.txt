cmake_minimum_required(VERSION 3.13)

project(cheeky LANGUAGES CXX VERSION 0.1.0)

add_subdirectory("third_party/fastgltf")
add_subdirectory("third_party/imgui")
add_subdirectory("third_party/imguizmo")
add_subdirectory("third_party/SDL")
add_subdirectory("third_party/vk-bootstrap")
add_subdirectory("third_party/volk")
add_subdirectory("third_party/glm")
add_subdirectory("third_party/VulkanMemoryAllocator")
add_subdirectory("third_party/stb")
add_subdirectory("third_party/onetbb")
add_subdirectory("third_party/flecs")

add_subdirectory("cheeky-engine")

# shaders!
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/data/shader/*.frag"
        "${PROJECT_SOURCE_DIR}/data/shader/*.vert"
        "${PROJECT_SOURCE_DIR}/data/shader/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/data/shader/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    message(STATUS COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV})
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)