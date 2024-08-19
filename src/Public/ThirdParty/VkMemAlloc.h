#pragma once

// We need to use this header to include third party projects that might have warnings in them.

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif