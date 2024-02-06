project('vkbootstrap', 'cpp')

sources = files('VkBootstrap.cpp')

vkbootstrap = library(
    'vkbootstrap',
    sources
)

vkbootstrap_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: vkbootstrap
)