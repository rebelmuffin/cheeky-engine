#pragma once

#include "Game/GameTime.h"
#include "Game/Node.h"
#include "Renderer/Scene.h"
#include "Renderer/VkEngine.h"

#include <memory>
#include <unordered_map>

namespace Game
{
    /// A GameScene defines a hierarchy of nodes and a render scene.
    /// Each node has a set of children and a single parent (except for the root).
    class GameScene
    {
      public:
        GameScene(Renderer::VulkanEngine& renderer, Renderer::Scene* scene);
        GameScene(const GameScene& other) = delete; // no copy
        GameScene(GameScene&& other) = default;
        GameScene& operator=(GameScene&& other) = default;

        void RegisterNode(Node& node);
        void ReleaseNode(Node& node);
        void SetActiveCamera(CameraNode* camera);
        void SetNodeTickUpdate(Node& node, bool update);
        void SetPaused(bool paused);
        Node* NodeFromId(NodeId_t node_id);
        RootNode& Root() { return *m_root.get(); }
        Renderer::Scene* RenderScene() { return m_render_scene; }
        Renderer::VulkanEngine& RenderEngine() { return *m_render_engine; }

        void Draw(const GameTime& time);

      private:
        void UpdateAllNodes(const GameTime& time);

        Renderer::VulkanEngine* m_render_engine;
        Renderer::Scene* m_render_scene;

        std::unordered_map<NodeId_t, Node*> m_active_nodes{};
        std::vector<Node*> m_updating_nodes;

        std::unique_ptr<RootNode> m_root;
        CameraNode* m_active_camera;

        bool m_paused = false;

        NodeId_t m_next_node_id = 1; // starting from 1 to avoid invalid node id
    };
} // namespace Game