project('Dear ImGUI', 'cpp', version: '1.19.1', default_options : ['default_library=static'])

imguiInc = include_directories('./backends', '.')

vulkan_dep = dependency('vulkan')
sdl2_dep = dependency('sdl2')
threadDep = dependency('threads')

sources = [
    'imgui.cpp',
    'imgui_draw.cpp',
    'imgui_tables.cpp',
    'imgui_demo.cpp',
    'imgui_widgets.cpp',
    './backends/imgui_impl_sdl2.cpp',
    './backends/imgui_impl_vulkan.cpp']

libImGUI = library('ImGUI', sources, include_directories : imguiInc, dependencies: [sdl2_dep, threadDep, vulkan_dep])
libImGUIDep = declare_dependency(link_with : libImGUI, include_directories : imguiInc)
meson.override_dependency('imgui', libImGUIDep)