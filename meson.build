project(
    'cheeky-vk-new',
    'cpp',
    version: '1.0.0',
    default_options: ['buildtype=debug', 'optimization=0', 'cpp_std=c++20'],
    meson_version: '>= 0.60.0',
)

cmake = import('cmake')

# Compile all shaders under data/shader/
# WARNING: All shaders need to have unique names
# #TODO: This should probably be done at runtime
glslang = find_program('glslangValidator')
shader_grabber = run_command('shader_grabber.sh', check: true)
shaders = shader_grabber.stdout().strip().split('\n')
shader_gen = generator(
    glslang,
    output: '@PLAINNAME@.spv',
    arguments: ['-V', '@INPUT@', '-o', '@INPUT@.spv'],
)
shader_gen_src = shader_gen.process(shaders)

# sdl2
vulkan_dep = dependency('vulkan')
sdl2_dep = dependency('sdl2')

vma_dep = dependency('vulkan-memory-allocator')
glm_dep = dependency('glm')
imgui_dep = dependency('imgui')

# cmake dependencies
vkbootstrap_proj = cmake.subproject('vkbootstrap')
vkbootstrap_dep = vkbootstrap_proj.dependency('vk-bootstrap')
volk_proj = cmake.subproject('volk')
volk_dep = volk_proj.dependency('volk')

# other third party
stb_image_dep = dependency('stb_image')
tinyobjloader_dep = dependency('tinyobjloader')

# includes
includes = include_directories('src/Public')

executable(
    'cheeky-vk-new',
    'src/Private/main.cpp',
    'src/Private/VkEngine.cpp',
    'src/Private/EngineCore.cpp',
    'src/Private/Utility/VkInitialisers.cpp',
    'src/Private/Utility/VkImages.cpp',
    'src/Private/Utility/DeletionQueue.cpp',
    shader_gen_src,
    dependencies: [
        vulkan_dep,
        sdl2_dep,
        vma_dep,
        glm_dep,
        imgui_dep,
        vkbootstrap_dep,
        volk_dep,
        stb_image_dep,
        tinyobjloader_dep,
    ],
    include_directories: includes,
)