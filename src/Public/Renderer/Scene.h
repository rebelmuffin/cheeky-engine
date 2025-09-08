#pragma once

#include "Renderer/Renderable.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/VkTypes.h"

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

namespace Renderer
{
    /// Structure that contains all the necessary information to render a single viewport and everything in
    /// it.
    struct Scene
    {
        // copy ctors need to be deleted for scene_items.
        Scene() = default;
        Scene(const Scene&) = delete;
        Scene(Scene&&) = default;
        Scene& operator=(const Scene&) = delete;
        Scene& operator=(Scene&&) = default;

        ImageHandle depth_image;
        ImageHandle draw_image;
        DrawContext frame_context; // gets reset every frame for a new draw.
        std::vector<std::unique_ptr<SceneItem>> scene_items;

        // if true, a clear command will be issued to clear the draw image every frame.
        bool clear_before_draw = true;

        // if you want to draw multiple scenes onto the same textures, use the viewport options below. If
        // zero, will cover the entire draw_image.
        glm::vec2 viewport_position;
        glm::vec2 viewport_extent;
        float render_scale = 1.0f;

        VkExtent2D draw_extent; // calculated every frame from image size and render scale.
        std::string scene_name;
    };
} // namespace Renderer