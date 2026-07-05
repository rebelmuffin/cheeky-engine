#include "Renderer/Utility/Camera.h"

namespace Renderer
{
    void Camera::Setup(const CameraSetupParams& params)
    {
        view = glm::mat4(params.rotation) * glm::translate(glm::mat4(1.0f), -params.position);

        // intentionally swapped near and far here. The renderer uses reversed depth.
        projection = glm::perspectiveFovZO(params.fov_rad, params.width, params.height, params.far_z, params.near_z);
    }
} // namespace Renderer