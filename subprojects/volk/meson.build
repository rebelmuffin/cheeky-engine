project('volk', ['c', 'cpp'])

sources = files('volk.c')

volk = library(
    'volk',
    sources
)

volk_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: volk
)