#pragma once

#include <glm/fwd.hpp>

struct SDL_Window;

namespace EngineUtils
{
    /// Return whether the given global pixel position in within the window.
    bool IsPointWithinWindow(SDL_Window* window, const glm::ivec2 point);

    float NormaliseAngleRadians(float theta);
} // namespace EngineUtils