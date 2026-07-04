#pragma once

// We need to use this header to include third party projects that might have warnings in them.

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui_internal.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif