// a single translation unit should include vk_mem_alloc with VMA_IMPLEMENTATION for linking
#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// to support logging memory leak errors
#ifdef CHEEKY_ENABLE_MEMORY_TRACKING

#include <cstdarg>
#include <iostream>

inline void VmaLeakLog(const char* format, ...)
{
    static char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, 1024, format, args);
    std::cout << "[!] VMA - " << buffer << std::endl;
    va_end(args);
}

#define VMA_LEAK_LOG_FORMAT(format, ...)                                                                               \
    {                                                                                                                  \
        VmaLeakLog(format, __VA_ARGS__);                                                                               \
    }
#endif

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif