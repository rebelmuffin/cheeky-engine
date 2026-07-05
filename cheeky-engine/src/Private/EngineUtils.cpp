#include "EngineUtils.h"

#include <SDL3/SDL.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/vec2.hpp>

#include <cmath>

namespace EngineUtils
{
    float NormaliseAngleRadians(float theta)
    {
        constexpr float two_pi = glm::pi<float>() * 2.0f;
        return theta - two_pi * std::floor((theta + glm::pi<float>()) / two_pi);
    }
} // namespace EngineUtils