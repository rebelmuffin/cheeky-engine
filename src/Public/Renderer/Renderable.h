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