#include "Game/Editor/SceneEditor.h"
#include "EngineUtils.h"
#include "Game/GameScene.h"
#include "Game/Node.h"

#include "ThirdParty/ImGUI.h"
#include <ImGuizmo.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>

namespace Game::Editor
{
    SceneEditor::SceneEditor(GameScene& scene) : m_scene(&scene) {}

    void SceneEditor::Draw(
        const double delta_time_seconds, Renderer::Window* window, Renderer::Viewport& editor_viewport
    )
    {
        const glm::quat rotation = glm::angleAxis(m_editor_camera.pitch_rad, glm::vec3(1, 0, 0)) *
                                   glm::angleAxis(m_editor_camera.yaw_rad, glm::vec3(0, 1, 0));

        const float width = editor_viewport.draw_image->image_extent.width;
        const float height = editor_viewport.draw_image->image_extent.height;
        m_editor_camera.render_camera.Setup(
            Renderer::CameraSetupParams{ rotation,
                                         m_editor_camera.position,
                                         width,
                                         height,
                                         glm::radians(m_editor_camera.vertical_fov_deg),
                                         m_camera_near,
                                         m_camera_far }
        );

        HandleInput(delta_time_seconds, window);
    }

    void SceneEditor::DrawImGui()
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

        const ImVec2 viewport_size = ImGui::GetMainViewport()->WorkSize;
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
            if (ImGui::CollapsingHeader("Camera"))
            {
                ImGui::SliderAngle("Yaw", &m_editor_camera.yaw_rad);
                ImGui::SliderAngle("Pitch", &m_editor_camera.pitch_rad);
                float fov_rad = glm::radians(m_editor_camera.vertical_fov_deg);
                if (ImGui::SliderAngle("Vertical FOV", &fov_rad))
                {
                    m_editor_camera.vertical_fov_deg = glm::degrees(fov_rad);
                }
                ImGui::DragFloat3("Position", &m_editor_camera.position.x);

                ImGui::DragFloat("Near Z", &m_camera_near);
                ImGui::DragFloat("Far Z", &m_camera_far);
            }
            DrawNodeHierarchy();
        }
        ImGui::End();

        if (m_enable_transform_gizmos && selected_node != nullptr)
        {
            DrawTransformGizmos(*selected_node);
        }

        for (const NodeId_t node_id : m_nodes_to_delete)
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

    void SceneEditor::HandleMouseInput(const double delta_time_seconds, Renderer::Window* window)
    {
        const float camera_mouse_rotation_multiplier = 1.0f;

        const glm::vec2 last_mouse_pos = m_last_mouse_pos;
        glm::vec2 mouse_pos;
        const uint32_t mouse_button_mask = SDL_GetGlobalMouseState(&mouse_pos.x, &mouse_pos.y);
        m_last_mouse_pos = mouse_pos;

        if (mouse_button_mask & SDL_BUTTON_RMASK && ImGui::GetIO().WantCaptureMouse == false)
        {
            glm::ivec2 mouse_delta{ 0 };
            if (window->IsPixelPositionInWindow(mouse_pos.x, mouse_pos.y) &&
                window->IsPixelPositionInWindow(last_mouse_pos.x, last_mouse_pos.y))
            {
                mouse_delta = mouse_pos - last_mouse_pos;
            }

            mouse_delta *= camera_mouse_rotation_multiplier;
            m_editor_camera.yaw_rad += (float)delta_time_seconds * (float)mouse_delta.x;
            const float pitch_delta = (float)delta_time_seconds * (float)mouse_delta.y;
            m_editor_camera.pitch_rad += pitch_delta;

            window->SetCaptureMouse(true);
        }
        else
        {
            window->SetCaptureMouse(false);
        }
    }

    void SceneEditor::HandleKeyboardInput(const double delta_time_seconds)
    {
        const float camera_rotation_rad_per_sec = glm::radians(110.0f);
        const float camera_movement_unit_per_sec = 1.0f;

        // Get camera basis vectors directly from rotation
        glm::mat4 view = m_editor_camera.render_camera.view;

        // Transform base vectors by rotation
        const glm::vec3 camera_up = glm::vec3(view[0][1], view[1][1], view[2][1]);
        const glm::vec3 camera_right = glm::vec3(view[0][0], view[1][0], view[2][0]);
        const glm::vec3 camera_forward = -glm::vec3(view[0][2], view[1][2], view[2][2]);

        const bool* key_states = SDL_GetKeyboardState(nullptr);

        // WASD for forward/right axis movement
        if (key_states[SDL_SCANCODE_W])
        {
            m_editor_camera.position +=
                camera_forward * (float)delta_time_seconds * camera_movement_unit_per_sec;
        }
        if (key_states[SDL_SCANCODE_S])
        {
            m_editor_camera.position -=
                camera_forward * (float)delta_time_seconds * camera_movement_unit_per_sec;
        }
        if (key_states[SDL_SCANCODE_D])
        {
            m_editor_camera.position +=
                camera_right * (float)delta_time_seconds * camera_movement_unit_per_sec;
        }
        if (key_states[SDL_SCANCODE_A])
        {
            m_editor_camera.position -=
                camera_right * (float)delta_time_seconds * camera_movement_unit_per_sec;
        }

        // Q/E for down/up
        if (key_states[SDL_SCANCODE_E])
        {
            m_editor_camera.position += camera_up * (float)delta_time_seconds * camera_movement_unit_per_sec;
        }
        if (key_states[SDL_SCANCODE_Q])
        {
            m_editor_camera.position -= camera_up * (float)delta_time_seconds * camera_movement_unit_per_sec;
        }

        // arrow keys for camera rotation without mouse input
        if (key_states[SDL_SCANCODE_UP])
        {
            m_editor_camera.pitch_rad -= camera_rotation_rad_per_sec * (float)delta_time_seconds;
        }
        if (key_states[SDL_SCANCODE_DOWN])
        {
            m_editor_camera.pitch_rad += camera_rotation_rad_per_sec * (float)delta_time_seconds;
        }
        if (key_states[SDL_SCANCODE_RIGHT])
        {
            m_editor_camera.yaw_rad += camera_rotation_rad_per_sec * (float)delta_time_seconds;
        }
        if (key_states[SDL_SCANCODE_LEFT])
        {
            m_editor_camera.yaw_rad -= camera_rotation_rad_per_sec * (float)delta_time_seconds;
        }
    }

    void SceneEditor::HandleInput(const double delta_time_seconds, Renderer::Window* window)
    {
        if (ImGui::GetIO().WantCaptureMouse == false)
        {
            HandleMouseInput(delta_time_seconds, window);
        }

        if (ImGui::GetIO().WantCaptureKeyboard == false)
        {
            HandleKeyboardInput(delta_time_seconds);
        }

        const float max_pitch = glm::radians(89.0f);
        const float min_pitch = glm::radians(-89.0f);

        m_editor_camera.yaw_rad = EngineUtils::NormaliseAngleRadians(m_editor_camera.yaw_rad);
        m_editor_camera.pitch_rad = glm::clamp(m_editor_camera.pitch_rad, min_pitch, max_pitch);
    }

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
        const ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle;
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
        bool local = false;
        if (ImGui::BeginTabBar("local_world_transform_bar"))
        {
            if (ImGui::BeginTabItem("Local"))
            {
                local = true;
                transform_changed |= ImGui::DragFloat3("Position", &cached_node.local_position.x);
                transform_changed |= ImGui::DragFloat3("Scale", &cached_node.local_scale.x, 0.5f, 0.01f);
                transform_changed |= ImGui::SliderAngle("Yaw", &cached_node.local_euler_rot.y);
                transform_changed |= ImGui::SliderAngle("Pitch", &cached_node.local_euler_rot.x);
                transform_changed |= ImGui::SliderAngle("Roll", &cached_node.local_euler_rot.z);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("World"))
            {
                local = false;
                transform_changed |= ImGui::DragFloat3("Position", &cached_node.world_position.x);
                transform_changed |= ImGui::DragFloat3("Scale", &cached_node.world_scale.x, 0.5f, 0.01f);
                transform_changed |= ImGui::SliderAngle("Yaw", &cached_node.world_euler_rot.y);
                transform_changed |= ImGui::SliderAngle("Pitch", &cached_node.world_euler_rot.x);
                transform_changed |= ImGui::SliderAngle("Roll", &cached_node.world_euler_rot.z);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (transform_changed)
        {
            if (local)
            {
                ApplyLocalCachedTransform(cached_node, node);
            }
            else
            {
                ApplyWorldCachedTransform(cached_node, node);
            }
        }

        node.OnImGui();
    }

    void SceneEditor::DrawTransformGizmos(Node& node)
    {
        glm::mat4 transform = node.WorldTransform().ToMatrix();
        const ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        if (ImGuizmo::Manipulate(
                &m_editor_camera.render_camera.view[0][0],
                &m_editor_camera.render_camera.projection[0][0],
                ImGuizmo::OPERATION::UNIVERSAL,
                ImGuizmo::MODE::LOCAL,
                &transform[0][0]
            ))
        {
            node.SetWorldTransform(Transform::FromMatrix(transform));
            ResetCachedNode(node); // since we replace the transform directly, we need to do this.
        }
    }

    void SceneEditor::SelectNode(Node& node)
    {
        m_selected_node = node.Id();
        ResetCachedNode(node);
    }

    void SceneEditor::ResetCachedNode(Node& node)
    {
        EditorCachedNode cached_node;
        ResetCachedNodeLocal(cached_node, node);
        ResetCachedNodeWorld(cached_node, node);
        m_cached_nodes[node.Id()] = cached_node;
    }

    void SceneEditor::ResetCachedNodeLocal(EditorCachedNode& cached_node, Node& node)
    {
        const Transform& local_xform = node.LocalTransform();
        cached_node.local_position = local_xform.position;
        cached_node.local_scale = local_xform.scale;
        cached_node.local_euler_rot = glm::eulerAngles(local_xform.rotation);
    }
    void SceneEditor::ResetCachedNodeWorld(EditorCachedNode& cached_node, Node& node)
    {
        const Transform& world_xform = node.WorldTransform();
        cached_node.world_position = world_xform.position;
        cached_node.world_scale = world_xform.scale;
        cached_node.world_euler_rot = glm::eulerAngles(world_xform.rotation);
    }

    void SceneEditor::ApplyLocalCachedTransform(EditorCachedNode& cached_node, Node& node)
    {
        const Transform xform{ cached_node.local_position,
                               cached_node.local_scale,
                               glm::quat{ cached_node.local_euler_rot } };
        node.SetLocalTransform(xform);

        // when we update local, we need to reset the world transform cache.
        ResetCachedNodeWorld(cached_node, node);
    }
    void SceneEditor::ApplyWorldCachedTransform(EditorCachedNode& cached_node, Node& node)
    {
        const Transform xform{ cached_node.world_position,
                               cached_node.world_scale,
                               glm::quat{ cached_node.world_euler_rot } };
        node.SetWorldTransform(xform);

        // when we update world, we need to reset the local transform cache.
        ResetCachedNodeLocal(cached_node, node);
    }
} // namespace Game::Editor