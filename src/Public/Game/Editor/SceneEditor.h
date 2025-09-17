#pragma once

#include "Game/GameScene.h"
#include "Game/Node.h"
#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"
#include <glm/ext/scalar_constants.hpp>

namespace Game::Editor
{
    /// Structure used for storing the information relating to a single node for editor purposes.
    struct EditorCachedNode
    {
        glm::vec3 local_position;
        glm::vec3 local_scale;
        glm::vec3 local_euler_rot{};

        glm::vec3 world_position;
        glm::vec3 world_scale;
        glm::vec3 world_euler_rot{};
    };

    struct EditorCamera
    {
        Renderer::Camera render_camera{};
        glm::vec3 position;
        float yaw_rad = glm::pi<float>();
        float pitch_rad;
        float vertical_fov_deg = 70.0f;
    };

    /// Class that represents an editor instance for a game scene.
    class SceneEditor
    {
      public:
        SceneEditor(GameScene& scene);

        void Draw(double delta_time_seconds, SDL_Window* window, Renderer::Viewport& editor_viewport);
        void DrawImGui();

        Renderer::Camera& Camera();
        bool EditorCameraEnabled();

      private:
        void HandleInput(double delta_time_seconds, SDL_Window* window);
        void DrawNodeEntry(Node& node);
        void DrawNodeHierarchy();
        void DrawNodeInspector(EditorCachedNode& cached_node, Node& node);
        void DrawTransformGizmos(Node& node);

        void SelectNode(Node& node);
        void ResetCachedNode(Node& node);
        void ResetCachedNodeLocal(EditorCachedNode& cached_node, Node& node);
        void ResetCachedNodeWorld(EditorCachedNode& cached_node, Node& node);
        void ApplyLocalCachedTransform(EditorCachedNode& cached_node, Node& node);
        void ApplyWorldCachedTransform(EditorCachedNode& cached_node, Node& node);

        GameScene* m_scene;

        bool m_enable_transform_gizmos = true;
        bool m_node_inspector_open = true;
        NodeId_t m_selected_node = INVALID_NODE_ID;
        std::vector<NodeId_t> m_nodes_to_delete{};
        std::unordered_map<NodeId_t, EditorCachedNode> m_cached_nodes{};

        // Camera
        EditorCamera m_editor_camera{};
        bool m_editor_camera_enabled = true;

        // inputs
        glm::ivec2 m_last_mouse_pos{ 0, 0 };
    };
} // namespace Game::Editor