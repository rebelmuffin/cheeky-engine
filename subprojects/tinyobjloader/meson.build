project('tinyobjloader', 'cpp')

sources = files('tiny_obj_loader.cc')

tinyobjloader = library(
    'tinyobjloader',
    sources
)

tinyobjloader_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: tinyobjloader
)