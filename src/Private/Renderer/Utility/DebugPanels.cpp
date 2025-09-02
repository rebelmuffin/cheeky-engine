#include "Renderer/Utility/DebugPanels.h"
#include "Renderer/Renderable.h"
#include "Renderer/ResourceStorage.h"
#include "Renderer/Scene.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"

#include "ThirdParty/ImGUI.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "imgui.h"
#include <glm/ext/matrix_transform.hpp>
#include <memory>

namespace Renderer::Debug
{
    void DrawSceneContentsImGui(Scene& scene)
    {
        ImGui::Text("Draw Resolution: %dx%d", scene.draw_extent.width, scene.draw_extent.height);
        ImGui::SliderFloat("Render Scale", &scene.render_scale, 0.1f, 1.0f);

        int item_to_delete = -1;
        int item_to_clone = -1;
        ImGui::PushID(scene.scene_name.data());
        if (ImGui::TreeNode("scene_contents", "Items: %zu", scene.scene_items.size()))
        {
            int item_idx = 0;
            for (std::unique_ptr<SceneItem>& item : scene.scene_items)
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
        ImGui::PopID();

        if (item_to_delete != -1)
        {
            scene.scene_items.erase(scene.scene_items.begin() + item_to_delete);
        }
        if (item_to_clone != -1)
        {
            scene.scene_items.emplace_back(scene.scene_items[(size_t)item_to_clone]->Clone());
        }
    }

    template <typename T>
    void DrawStorageTableImGui(
        ResourceStorage<T>& storage,
        std::function<void()> setup_custom_columns,
        std::function<void(const T&, int)> draw_resource_info,
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
                draw_resource_info(resource, 1);
            }
            ImGui::EndTable();
        }
    }

    void DrawStorageTableImGui(ResourceStorage<AllocatedImage>& image_storage)
    {
        constexpr int custom_column_count = 2;
        DrawStorageTableImGui<AllocatedImage>(
            image_storage,
            []
            {
                ImGui::TableSetupColumn("Extents");
                ImGui::TableSetupColumn("Format");
            },
            [](const AllocatedImage& img, int last_column)
            {
                ImGui::TableSetColumnIndex(last_column + 1);
                ImGui::Text("%dx%d", img.image_extent.width, img.image_extent.height);
                ImGui::TableSetColumnIndex(last_column + 2);
                ImGui::Text("%s", string_VkFormat(img.image_format));
            },
            custom_column_count
        );
    }

    void DrawStorageTableImGui(ResourceStorage<AllocatedBuffer>& buffer_storage)
    {
        constexpr int custom_column_count = 2;
        DrawStorageTableImGui<AllocatedBuffer>(
            buffer_storage,
            []()
            {
                ImGui::TableSetupColumn("Size");
                ImGui::TableSetupColumn("Address");
            },
            [](const AllocatedBuffer& buf, int last_column)
            {
                ImGui::TableSetColumnIndex(last_column + 1);
                ImGui::Text("%zu bytes", buf.allocation_info.size);
                ImGui::TableSetColumnIndex(last_column + 2);
                ImGui::Text("0x%p", buf.buffer);
            },
            custom_column_count
        );
    }

    void DrawStorageTableImGui(ResourceStorage<MeshAsset>& mesh_storage)
    {
        constexpr int custom_column_count = 1;
        DrawStorageTableImGui<MeshAsset>(
            mesh_storage,
            []()
            {
                ImGui::TableSetupColumn("Surface Count");
            },
            [](const MeshAsset& mesh, int last_column)
            {
                ImGui::TableSetColumnIndex(last_column + 1);
                ImGui::Text("%zu", mesh.surfaces.size());
            },
            custom_column_count
        );
    }
} // namespace Renderer::Debug