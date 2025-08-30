project(
    'cheeky-vk-new',
    'cpp',
    version: '1.0.0',
    default_options: ['buildtype=debug', 'optimization=0', 'cpp_std=c++20'],
    meson_version: '>= 0.60.0',
)

add_global_arguments('-DGLM_FORCE_DEPTH_ZERO_TO_ONE', language: 'cpp')

add_project_arguments('-DCHEEKY_ENABLE_MEMORY_TRACKING', language: 'cpp')

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

subproject('imgui')
imgui_dep = dependency('imgui')

# cmake dependencies
vkbootstrap_proj = cmake.subproject('vkbootstrap')
vkbootstrap_dep = vkbootstrap_proj.dependency('vk-bootstrap')

# other third party
stb_image_dep = dependency('stb_image')
tinyobjloader_dep = dependency('tinyobjloader')
fastgltf_proj = cmake.subproject('fastgltf')
fastgltf_dep = fastgltf_proj.dependency('fastgltf')

# includes
includes = include_directories('src/Public', 'subprojects') # is this okay?

executable(
    'cheeky-vk-new',
    'src/Private/main.cpp',
    'src/Private/EngineCore.cpp',
    'src/Private/Renderer/VkEngine.cpp',
    'src/Private/Renderer/Material.cpp',
    'src/Private/Renderer/Utility/VkLoader.cpp',
    'src/Private/Renderer/Utility/VkPipelines.cpp',
    'src/Private/Renderer/Utility/VkInitialisers.cpp',
    'src/Private/Renderer/Utility/VkImages.cpp',
    'src/Private/Renderer/Utility/VkDescriptors.cpp',
    'src/Private/Renderer/Utility/DeletionQueue.cpp',
    'src/Private/Renderer/Utility/UploadRequest.cpp',
    'src/Private/ThirdParty/VkMemAllocImpl.cpp',
    shader_gen_src,
    dependencies: [
        vulkan_dep,
        sdl2_dep,
        vma_dep,
        glm_dep,
        imgui_dep,
        vkbootstrap_dep,
        stb_image_dep,
        tinyobjloader_dep,
        fastgltf_dep
    ],
    include_directories: includes,
)
