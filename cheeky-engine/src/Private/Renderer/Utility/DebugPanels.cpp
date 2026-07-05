#include "Renderer/Utility/DebugPanels.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"
#include "Renderer/VkTypes.h"

#include "ThirdParty/ImGUI.h"
#include "imgui_internal.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>

#include <cstring>
#include <filesystem>
#include <limits>
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

        static float camera_fov_rad = glm::radians(70.0f);
        static float camera_yaw_rad = 0.0f;
        static float camera_pitch_rad = 0.0f;
        static float camera_far_plane = 10000.0f;
        static float camera_near_plane = 0.1f;
        static glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, -1.0f);
        if (ImGui::CollapsingHeader("Override Camera Settings"))
        {
            ImGui::SliderAngle("Vertical FOV", &camera_fov_rad, 1.0f, 179.0f);
            ImGui::SliderAngle("Camera yaw", &camera_yaw_rad);
            ImGui::SliderAngle("Camera pitch", &camera_pitch_rad, -89.0f, 89.0f);
            ImGui::DragFloat3("Camera position", &camera_pos.x);

            float max_float = std::numeric_limits<float>::max();
            ImGui::DragFloat("Far Plane", &camera_far_plane, 10.0f, 0.01f, max_float, "%.2f");
            ImGui::DragFloat("Near Plane", &camera_near_plane, 10.0f, 0.01f, max_float, "%.2f");

            // pitch first, then yaw. order of multiplication matters
            glm::mat4 rotation = glm::rotate(camera_pitch_rad, glm::vec3(1, 0, 0)) *
                                 glm::rotate(camera_yaw_rad, glm::vec3(0, 1, 0));

            const float width = viewport.draw_image->image_extent.width;
            const float height = viewport.draw_image->image_extent.height;
            viewport.frame_context.camera.Setup(
                { rotation, camera_pos, width, height, camera_fov_rad, camera_near_plane, camera_far_plane }
            );
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
                ImGui::Text("%zu bytes", static_cast<size_t>(buf.allocation_info.size));
                ImGui::TableSetColumnIndex(last_column + 2);
                ImGui::Text("0x%p", reinterpret_cast<void*>(buf.buffer));
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