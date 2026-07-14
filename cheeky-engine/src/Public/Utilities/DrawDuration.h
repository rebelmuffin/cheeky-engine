#pragma once

namespace Debug
{
    struct DrawDuration
    {
        double seconds{};
    };
    static DrawDuration ONE_FRAME = { 0.0f };
} // namespace Debug