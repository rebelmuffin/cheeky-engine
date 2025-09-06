#pragma once

#include "Game/Node.h"
#include "Renderer/Renderable.h"
#include "Renderer/Utility/VkLoader.h"

namespace Game
{
    class MeshNode : public Node
    {
      public:
        MeshNode(const Renderer::MeshHandle& handle);

        void OnAdded() override;
        void OnRemoved() override;
        void OnTickUpdate(const GameTime&) override;

      private:
        Renderer::MeshHandle m_mesh_asset;
        Renderer::MeshSceneItem* m_scene_item;
    };
} // namespace Game