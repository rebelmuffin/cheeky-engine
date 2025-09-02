#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <cstdarg>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif