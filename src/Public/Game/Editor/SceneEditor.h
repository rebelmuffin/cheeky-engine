#pragma once

#include "Game/GameScene.h"
#include "Game/Node.h"

namespace Game::Editor
{
    /// Class that represents an editor instance for a game scene.
    class SceneEditor
    {
      public:
        SceneEditor(GameScene& scene);

        void DrawImGui();

      private:
        void DrawNodeEntry(Node& node);
        void DrawNodeHierarchy();
        void DrawNodeInspector(Node& node);
        void DrawTransformGizmos(Node& node);

        GameScene* m_scene;

        bool m_enable_transform_gizmos = true;
        bool m_node_inspector_open = true;
        NodeId_t m_selected_node = INVALID_NODE_ID;
        std::vector<NodeId_t> m_nodes_to_delete{};
    };
} // namespace Game::Editor