#pragma once

#include <cstdint>
#include <string_view>

struct SDL_Window;

namespace EngineUtils
{
    float NormaliseAngleRadians(float theta);

    uint32_t Hash(uint32_t val);
    uint64_t Hash(uint64_t val);
    uint64_t Hash(std::string_view str, uint32_t seed = 0);
} // namespace EngineUtils