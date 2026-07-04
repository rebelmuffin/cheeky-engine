#pragma once

#include "Renderer/RenderObject.h"
#include "Renderer/Utility/Camera.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"
#include <memory>

namespace Renderer
{
    class VulkanEngine;

    struct FrameDrawContext
    {
        std::vector<RenderObject> render_objects;

        Camera camera;

        // environment data
        glm::vec4 ambient_colour = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        glm::vec4 light_direction = glm::vec4(0.34f, 0.33f, 0.33f, 0.0f);
        glm::vec4 light_colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    };
} // namespace Renderer