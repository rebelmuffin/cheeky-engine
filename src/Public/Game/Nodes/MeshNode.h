#pragma once

#include "Game/Node.h"
#include "Renderer/Utility/VkLoader.h"

namespace Renderer
{
    struct FrameDrawContext;
}

namespace Game
{
    class MeshNode : public Node
    {
      public:
        MeshNode(std::string_view name, const Renderer::MeshHandle& handle);

        void OnAdded() override;
        void OnRemoved() override;
        void Draw(Renderer::FrameDrawContext& ctx) override;

      private:
        Renderer::MeshHandle m_mesh_asset;
    };
} // namespace Game