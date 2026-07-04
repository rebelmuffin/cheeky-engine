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

add_subdirectory("cheeky-engine")