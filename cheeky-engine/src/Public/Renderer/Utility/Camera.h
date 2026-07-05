#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Renderer
{
    struct CameraSetupParams
    {
        const glm::quat& rotation;
        const glm::vec3& position;
        float width;
        float height{};
        float fov_rad;
        float near_z = 0.1f;
        float far_z = 10000.0f;
    };

    struct Camera
    {
        glm::mat4 view{ 1.0f };
        glm::mat4 projection{ 1.0f };

        void Setup(const CameraSetupParams& params);
    };
} // namespace Renderer