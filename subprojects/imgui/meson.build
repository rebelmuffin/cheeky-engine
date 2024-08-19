project('Dear ImGUI', 'cpp', version: '1.19.1')

imguiInc = include_directories('./backends', '.')


glslang = find_program('glslangValidator')
shaders = ['backends/vulkan/glsl_shader.frag', 'backends/vulkan/glsl_shader.vert']
shader_gen = generator(
    glslang,
    output: '@PLAINNAME@.u32',
    arguments: ['-V', 'x', '@INPUT@', '-o', '@INPUT@.u32'],
)
shader_gen_src = shader_gen.process(shaders)

sdl2_dep = dependency('sdl2')

sources = [
    'imgui.cpp',
    'imgui_draw.cpp',
    'imgui_tables.cpp',
    'imgui_demo.cpp',
    'imgui_widgets.cpp',
    './backends/imgui_impl_sdl2.cpp',
    './backends/imgui_impl_vulkan.cpp',
    shader_gen_src]

libImGUI = library('ImGUI', sources, include_directories : imguiInc, dependencies: [sdl2_dep])
libImGUIDep = declare_dependency(link_with : libImGUI, include_directories : imguiInc)