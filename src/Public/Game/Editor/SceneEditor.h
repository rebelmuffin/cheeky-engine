#pragma once

#include "Game/GameScene.h"
#include "Game/Node.h"

namespace Game::Editor
{
    /// Structure used for storing the information relating to a single node for editor purposes.
    struct EditorCachedNode
    {
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 euler_angles_rot{};
    };

    /// Class that represents an editor instance for a game scene.
    class SceneEditor
    {
      public:
        SceneEditor(GameScene& scene);

        void DrawImGui();

      private:
        void DrawNodeEntry(Node& node);
        void DrawNodeHierarchy();
        void DrawNodeInspector(EditorCachedNode& cached_node, Node& node);
        void DrawTransformGizmos(Node& node);

        void SelectNode(Node& node);
        void ResetCachedNode(Node& node);
        void ApplyCachedTransform(EditorCachedNode& cached_node, Node& node);

        GameScene* m_scene;

        bool m_enable_transform_gizmos = true;
        bool m_node_inspector_open = true;
        NodeId_t m_selected_node = INVALID_NODE_ID;
        std::vector<NodeId_t> m_nodes_to_delete{};
        std::unordered_map<NodeId_t, EditorCachedNode> m_cached_nodes{};
    };
} // namespace Game::Editor