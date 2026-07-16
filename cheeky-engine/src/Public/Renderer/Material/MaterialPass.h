#pragma once

#include <cstdint>

namespace Renderer
{
    enum class MaterialPass : uint8_t
    {
        MainColour,
        Transparent,
        NoDepth,
        Other
    };
} // namespace Renderer