#pragma once

#include "Renderer/Renderable.h"
#include "Renderer/VkTypes.h"

#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include <memory>

namespace Renderer
{
    /// Structure that contains all the necessary information to render a single viewport and everything in
    /// it.
    struct Scene
    {
        AllocatedImage depth_image;
        AllocatedImage draw_image;
        std::vector<std::unique_ptr<SceneItem>> scene_items;

        float camera_vertical_fov;
        glm::mat4 camera_rotation;
        glm::vec3 camera_position;

        // environment data
        glm::vec4 ambient_colour = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        glm::vec4 light_direction = glm::vec4(0.34f, 0.33f, 0.33f, 0.0f);
        glm::vec4 light_colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        // if you want to draw multiple scenes onto the same textures, use the viewport options below. If
        // zero, will cover the entire draw_image.
        glm::vec2 viewport_position;
        glm::vec2 viewport_extent;
        float render_scale;

        VkExtent2D draw_extent; // calculated every frame from image size and render scale.
        std::string scene_name;
    };
} // namespace Renderer