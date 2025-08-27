#pragma once

// We need to use this header to include third party projects that might have warnings in them.

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif