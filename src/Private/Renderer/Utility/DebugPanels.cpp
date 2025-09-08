#include "Renderer/Utility/DebugPanels.h"
#include "Renderer/Renderable.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"
#include "Renderer/VkTypes.h"

#include "ThirdParty/ImGUI.h"
#include "imgui_internal.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui.h>

#include <cstring>
#include <filesystem>
#include <memory>

namespace Renderer::Debug
{
    void DrawViewportContentsImGui(VulkanEngine& engine, Viewport& viewport)
    {
        if (&engine.active_viewports[engine.main_viewport] == &viewport)
        {
            ImGui::Text("This is the main viewport.");
        }
        else
        {
            if (ImGui::Button("Make main viewport"))
            {
                for (size_t i = 0; i < engine.active_viewports.size(); ++i)
                {
                    if (&engine.active_viewports[i] == &viewport)
                    {
                        engine.main_viewport = i;
                    }
                }
            }
        }

        ImGui::Text("Draw Resolution: %dx%d", viewport.draw_extent.width, viewport.draw_extent.height);
        ImGui::SliderFloat("Render Scale", &viewport.render_scale, 0.1f, 1.0f);

        static float camera_yaw_rad = 0.0f;
        static float camera_pitch_rad = 0.0f;
        static glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -1.0f);
        if (ImGui::CollapsingHeader("Camera Settings"))
        {
            ImGui::SliderAngle("Camera yaw", &camera_yaw_rad);
            ImGui::SliderAngle("Camera pitch", &camera_pitch_rad, -89.0f, 89.0f);
            ImGui::DragFloat3("Camera position", &camera_pos.x);

            // pitch first, then yaw. order of multiplication matters
            glm::mat4 rotation = glm::rotate(camera_pitch_rad, glm::vec3(1, 0, 0)) *
                                 glm::rotate(camera_yaw_rad, glm::vec3(0, 1, 0));

            viewport.frame_context.camera_position = camera_pos;
            viewport.frame_context.camera_rotation = rotation;
        }

        ImGui::Separator();
        static long int selected_asset = -1;
        static char filter[255];
        std::optional<std::filesystem::path> gltf_asset = std::nullopt;
        if (ImGui::InputText("Filter", filter, 255))
        {
            selected_asset = -1;
        }
        if (ImGui::BeginListBox("GLTF Assets"))
        {
            // enumerate all .gltf and .glb assets under resources
            const char* resources_path = "../data/resources";
            std::filesystem::recursive_directory_iterator dir_iter(
                resources_path, std::filesystem::directory_options::skip_permission_denied
            );

            std::vector<std::filesystem::path> paths{};
            for (std::filesystem::path file : dir_iter)
            {
                if (file.has_extension() &&
                    (file.extension().compare(".gltf") == 0 || file.extension().compare(".glb") == 0) &&
                    std::strstr(file.c_str(), filter) != nullptr)
                {
                    paths.emplace_back(file);
                }
            }

            for (size_t i = 0; i < paths.size(); ++i)
            {
                if (ImGui::Selectable(paths[i].c_str(), (size_t)selected_asset == i))
                {
                    selected_asset = (long int)i;
                }
            }

            if (selected_asset != -1)
            {
                gltf_asset = paths[(size_t)selected_asset];
            }

            ImGui::EndListBox();
        }
        ImGui::BeginDisabled(gltf_asset == std::nullopt);
        if (ImGui::Button("Load GLTF"))
        {
            Utils::LoadGltfIntoScene(viewport, engine, gltf_asset.value());
        }
        ImGui::EndDisabled();
        ImGui::Separator();

        int item_to_delete = -1;
        int item_to_clone = -1;
        if (ImGui::Button("Clear Viewport"))
        {
            viewport.scene_items.clear();
        }
        if (ImGui::TreeNode("viewport_contents", "Items: %zu", viewport.scene_items.size()))
        {
            int item_idx = 0;
            for (std::unique_ptr<SceneItem>& item : viewport.scene_items)
            {
                if (ImGui::TreeNode(item->name.data()))
                {
                    if (ImGui::Button("Delete"))
                    {
                        item_to_delete = item_idx;
                    }
                    if (ImGui::Button("Clone"))
                    {
                        item_to_clone = item_idx;
                    }
                    ImGui::Text("Name: %s", item->name.data());
                    glm::vec3 scale;
                    glm::quat rot;
                    glm::vec3 translation;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(item->transform, scale, rot, translation, skew, perspective);
                    ImGui::DragFloat3("Translation", &translation.x);
                    ImGui::DragFloat3("Scale", &scale.x, 1.0f, 0.01f);

                    glm::mat4 rotation = glm::mat4(rot);
                    item->transform = rotation * glm::translate(translation) * glm::scale(scale);

                    ImGui::TreePop();
                }
                item_idx++;
            }
            ImGui::TreePop();
        }

        if (item_to_delete != -1)
        {
            viewport.scene_items.erase(viewport.scene_items.begin() + item_to_delete);
        }
        if (item_to_clone != -1)
        {
            viewport.scene_items.emplace_back(viewport.scene_items[(size_t)item_to_clone]->Clone());
        }
    }

    template <typename T>
    void DrawStorageTableImGui(
        ResourceStorage<T>& storage,
        std::function<void()> setup_custom_columns,
        std::function<void(StorageId_t id, const T& resource, int last_column)> draw_resource_info,
        int custom_column_count
    )
    {
        if (ImGui::BeginTable(
                "ResourceTable",
                2 + custom_column_count,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_Sortable
            ))
        {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            setup_custom_columns();
            ImGui::TableHeadersRow();

            for (const auto& [id, resource] : storage.resource_map)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%zu", id);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", storage.resource_name_map[id].data());
                draw_resource_info(id, resource, 1);
            }
            ImGui::EndTable();
        }
    }

    void DrawStorageTableImGui(VulkanEngine& engine, ResourceStorage<AllocatedImage>& image_storage)
    {
        constexpr int custom_column_count = 3;
        DrawStorageTableImGui<AllocatedImage>(
            image_storage,
            []
            {
                ImGui::TableSetupColumn("Extents");
                ImGui::TableSetupColumn("Format");
                ImGui::TableSetupColumn("Image Contents");
            },
            [&](StorageId_t id, const AllocatedImage& img, int last_column)
            {
                ImGui::TableSetColumnIndex(last_column + 1);
                ImGui::Text("%dx%d", img.image_extent.width, img.image_extent.height);
                ImGui::TableSetColumnIndex(last_column + 2);
                ImGui::Text("%s", string_VkFormat(img.image_format));
                ImGui::TableSetColumnIndex(last_column + 3);

                ImageHandle img_handle = image_storage.HandleFromID(id);
                ImTextureID texture_id = engine.ImageDebugTextureId(img_handle);
                ImGui::Image(texture_id, ImVec2{ 48, 48 });
                if (ImGui::IsItemHovered())
                {
                    if (ImGui::BeginTooltip())
                    {
                        // keep the aspect ratio while showing a smaller image
                        float width_to_height =
                            (float)img_handle->image_extent.width / (float)img_handle->image_extent.height;
                        ImGui::Image(texture_id, ImVec2{ 256 * width_to_height, 256 });
                        ImGui::EndTooltip();
                    }
                }
            },
            custom_column_count
        );
    }

    void DrawStorageTableImGui(VulkanEngine&, ResourceStorage<AllocatedBuffer>& buffer_storage)
    {
        constexpr int custom_column_count = 2;
        DrawStorageTableImGui<AllocatedBuffer>(
            buffer_storage,
            []()
            {
                ImGui::TableSetupColumn("Size");
                ImGui::TableSetupColumn("Address");
            },
            [](StorageId_t, const AllocatedBuffer& buf, int last_column)
            {
                ImGui::TableSetColumnIndex(last_column + 1);
                ImGui::Text("%zu bytes", (size_t)buf.allocation_info.size);
                ImGui::TableSetColumnIndex(last_column + 2);
                ImGui::Text("0x%p", buf.buffer);
            },
            custom_column_count
        );
    }

    void DrawStorageTableImGui(VulkanEngine&, ResourceStorage<MeshAsset>& mesh_storage)
    {
        constexpr int custom_column_count = 1;
        DrawStorageTableImGui<MeshAsset>(
            mesh_storage,
            []()
            {
                ImGui::TableSetupColumn("Surface Count");
            },
            [](StorageId_t, const MeshAsset& mesh, int last_column)
            {
                ImGui::TableSetColumnIndex(last_column + 1);
                ImGui::Text("%zu", mesh.surfaces.size());
            },
            custom_column_count
        );
    }
} // namespace Renderer::Debug