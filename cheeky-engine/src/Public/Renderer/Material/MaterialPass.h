#pragma once

#include <cstdint>

namespace Renderer
{
    enum class MaterialPass : uint8_t
    {
        MainColour,
        Transparent,
        Other
    };
} // namespace Renderer