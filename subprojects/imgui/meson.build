project('Dear ImGUI', 'cpp', version: '1.92.2b', default_options : ['default_library=static'])

imgui_includes = include_directories('./backends', '.')

# imgui vulkan backend requires this. Otherwise it'll try to load vulkan functions without volk and that is no good
add_project_arguments('-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES', language: 'cpp')

vulkan_dep = dependency('vulkan')
sdl2_dep = dependency('sdl2')
thread_dep = dependency('threads')

sources = [
    'imgui.cpp',
    'imgui_draw.cpp',
    'imgui_tables.cpp',
    'imgui_demo.cpp',
    'imgui_widgets.cpp',
    './backends/imgui_impl_sdl2.cpp',
    './backends/imgui_impl_vulkan.cpp']

libImGUI = library('ImGUI', sources, include_directories : imgui_includes, dependencies: [sdl2_dep, thread_dep, vulkan_dep])
libImGUIDep = declare_dependency(link_with : libImGUI, include_directories : imgui_includes)
meson.override_dependency('imgui', libImGUIDep)