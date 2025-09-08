#pragma once

#include "Game/GameTime.h"
#include "Game/Node.h"
#include "Renderer/Renderable.h"
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
        GameScene();
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

        /// May get called multiple times per frame to draw the same scene on different views.
        /// If camera is specified, will force that camera for the draw, otherwise will use the active
        /// camera.
        void Draw(Renderer::FrameDrawContext& ctx, const CameraNode* camera_node = nullptr);

        /// Called once a frame for the logical update of the game.
        void TickUpdate(const GameTime& time);

      private:
        void SetNodeRenderable(Node& node, bool is_renderable);
        void UpdateAllNodes(const GameTime& time);

        std::unordered_map<NodeId_t, Node*> m_active_nodes{};
        std::vector<Node*> m_updating_nodes;
        std::vector<Node*> m_renderable_nodes;

        std::unique_ptr<RootNode> m_root;
        CameraNode* m_active_camera = nullptr;

        bool m_paused = false;

        NodeId_t m_next_node_id = 1; // starting from 1 to avoid invalid node id
    };
} // namespace Game