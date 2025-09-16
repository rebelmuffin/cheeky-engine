#include "Game/Editor/SceneEditor.h"
#include "Game/GameScene.h"
#include "Game/Node.h"

#include "ImGuizmo.h"
#include "ThirdParty/ImGUI.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace Game::Editor
{
    SceneEditor::SceneEditor(GameScene& scene) : m_scene(&scene) {}

    void SceneEditor::Draw(Renderer::Viewport& editor_viewport)
    {
        float aspect_ratio = (float)editor_viewport.draw_image->image_extent.width /
                             (float)editor_viewport.draw_image->image_extent.height;
        m_editor_camera.render_camera.view =
            glm::mat4(m_editor_camera.rotation) * glm::translate(m_editor_camera.position);
        m_editor_camera.render_camera.projection =
            glm::perspective(glm::radians(m_editor_camera.vertical_fov_deg), aspect_ratio, 10000.0f, 0.1f);
    }

    void SceneEditor::DrawImGui(Renderer::Viewport& editor_viewport)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Scene Editor"))
            {
                ImGui::Checkbox("Node Inspector", &m_node_inspector_open);
                ImGui::Checkbox("Transform Gizmos", &m_enable_transform_gizmos);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        Node* selected_node = m_scene->NodeFromId(m_selected_node);

        ImVec2 viewport_size = ImGui::GetMainViewport()->WorkSize;
        const float panel_width = viewport_size.x / 5.0f;
        const float panel_height = viewport_size.y;
        const float panel_pos_y = ImGui::GetMainViewport()->WorkPos.y;
        if (m_node_inspector_open)
        {
            // from the right
            ImGui::SetNextWindowPos(ImVec2(viewport_size.x - panel_width, panel_pos_y));
            ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

            if (ImGui::Begin("Node Inspector", &m_node_inspector_open))
            {
                if (selected_node != nullptr)
                {
                    DrawNodeInspector(m_cached_nodes[selected_node->Id()], *selected_node);
                }
                else
                {
                    ImGui::Text("Select a node in the node hierarchy to edit its contents.");
                }
            }
            ImGui::End();
        }

        // node hierarchy is on the left.
        ImGui::SetNextWindowPos(ImVec2(0.0f, panel_pos_y));
        ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

        if (ImGui::Begin("Scene Contents"))
        {
            if (ImGui::CollapsingHeader("Lighting"))
            {
                ImGui::Text("todo");
            }
            DrawNodeHierarchy();
        }
        ImGui::End();

        if (m_enable_transform_gizmos && selected_node != nullptr)
        {
            DrawTransformGizmos(editor_viewport, *selected_node);
        }

        for (NodeId_t node_id : m_nodes_to_delete)
        {
            Node* node = m_scene->NodeFromId(node_id);
            if (node == nullptr)
            {
                continue;
            }

            node->Destroy();
        }
        m_nodes_to_delete.clear();
    }

    Renderer::Camera& SceneEditor::Camera() { return m_editor_camera.render_camera; }
    bool SceneEditor::EditorCameraEnabled() { return m_editor_camera_enabled; }

    void SceneEditor::DrawNodeEntry(Node& node)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (node.Children().empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (m_selected_node == node.Id())
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (node.IsRootNode())
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        bool open = false;
        if (ImGui::TreeNodeEx(
                node.Name().data(),
                flags,
                "%u - %s (%zu)",
                node.Id(),
                node.Name().data(),
                node.Children().size()
            ))
        {
            open = true;
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                SelectNode(node);
            }

            for (const std::unique_ptr<Node>& child : node.Children())
            {
                DrawNodeEntry(*child);
            }

            ImGui::TreePop();
        }

        // we need this to be able to select tree nodes that aren't open but have children
        if (open == false && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            SelectNode(node);
        }
    }

    void SceneEditor::DrawNodeHierarchy()
    {
        // not using listbox because it has a weird shape.
        ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle;
        ImGui::Text("Nodes");
        if (ImGui::BeginChild("nodes_list", ImVec2{}, flags))
        {
            DrawNodeEntry(m_scene->Root());
            ImGui::EndChild();
        }
    }

    void SceneEditor::DrawNodeInspector(EditorCachedNode& cached_node, Node& node)
    {
        ImGui::Text("Name: %s", node.Name().data());
        ImGui::Text("Id: %u", node.Id());
        if (ImGui::Button("Delete"))
        {
            m_nodes_to_delete.emplace_back(node.Id());
        }

        bool transform_changed = false;
        transform_changed |= ImGui::DragFloat3("Position", &cached_node.position.x);
        transform_changed |= ImGui::DragFloat3("Scale", &cached_node.scale.x, 0.5f, 0.01f);
        transform_changed |= ImGui::SliderAngle("Yaw", &cached_node.euler_angles_rot.y);
        transform_changed |= ImGui::SliderAngle("Pitch", &cached_node.euler_angles_rot.x);
        transform_changed |= ImGui::SliderAngle("Roll", &cached_node.euler_angles_rot.z);

        if (transform_changed)
        {
            ApplyCachedTransform(cached_node, node);
        }

        node.OnImGui();
    }

    void SceneEditor::DrawTransformGizmos(Renderer::Viewport&, Node&)
    {
        // glm::mat4x4 transform = node.WorldTransform().ToMatrix();
        // ImGuizmo::Manipulate(
        //     const float* view, const float* projection, OPERATION operation, MODE mode, float* matrix
        // )
    }

    void SceneEditor::SelectNode(Node& node)
    {
        m_selected_node = node.Id();
        ResetCachedNode(node);
    }

    void SceneEditor::ResetCachedNode(Node& node)
    {
        const Transform& xform = node.LocalTransform();
        const glm::vec3 position = xform.position;
        const glm::vec3 scale = xform.scale;
        const glm::vec3 euler_angles = glm::eulerAngles(xform.rotation);
        m_cached_nodes[node.Id()] = EditorCachedNode{ position, scale, euler_angles };
    }

    void SceneEditor::ApplyCachedTransform(EditorCachedNode& cached_node, Node& node)
    {
        const Transform xform{ cached_node.position,
                               cached_node.scale,
                               glm::quat{ cached_node.euler_angles_rot } };
        node.SetLocalTransform(xform);
    }
} // namespace Game::Editor