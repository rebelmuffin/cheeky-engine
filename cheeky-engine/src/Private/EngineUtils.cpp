#include "EngineUtils.h"

#include <SDL3/SDL.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/vec2.hpp>

#include <cmath>

namespace EngineUtils
{
    bool IsPointWithinWindow(SDL_Window* window, const glm::ivec2 point)
    {
        int window_x, window_y;
        int window_w, window_h;
        SDL_GetWindowPosition(window, &window_x, &window_y);
        SDL_GetWindowSize(window, &window_w, &window_h);

        int max_x = window_x + window_w;
        int max_y = window_y + window_h;
        return point.x > window_x && point.x <= max_x && point.y > window_y && point.y <= max_y;
    }

    float NormaliseAngleRadians(float theta)
    {
        constexpr float two_pi = glm::pi<float>() * 2.0f;
        return theta - two_pi * std::floor((theta + glm::pi<float>()) / two_pi);
    }
} // namespace EngineUtils