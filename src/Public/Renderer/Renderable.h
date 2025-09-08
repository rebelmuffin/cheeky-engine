#pragma once

#include "Renderer/RenderObject.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"
#include <memory>

namespace Renderer
{
    class VulkanEngine;

    /// Interface that provides everything necessary to set up the renderables
    struct DrawEngineInterface
    {
        VulkanEngine* engine; // engine itself for public access
    };

    struct DrawContext
    {
        std::vector<RenderObject> render_objects;

        float camera_vertical_fov = 70.0f;
        glm::mat4 camera_rotation = glm::mat4(1.0f);
        glm::vec3 camera_position = glm::vec3(0.0f);

        // environment data
        glm::vec4 ambient_colour = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        glm::vec4 light_direction = glm::vec4(0.34f, 0.33f, 0.33f, 0.0f);
        glm::vec4 light_colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    };

    /// Base class of anything that can have an input to the renderer for drawing geometry.
    struct IRenderable
    {
        virtual ~IRenderable() = default;

        /// This is where the render objects need to be added to the draw context. The context will reset
        /// every frame.
        virtual void Draw(DrawContext& ctx) = 0;
    };

    /// Base class of all things that can be identified and rendered in a scene.
    struct SceneItem : public IRenderable
    {
        virtual ~SceneItem() = default;

        virtual std::unique_ptr<SceneItem> Clone() const = 0;

        glm::mat4 transform;
        std::string name;
    };

    struct MeshSceneItem : public SceneItem
    {
        virtual ~MeshSceneItem() = default;

        std::unique_ptr<SceneItem> Clone() const override;
        void Draw(DrawContext& ctx) override;

        MeshHandle asset;
    };
} // namespace Renderer