#include "Game/GameMain.h"
#include "Game/Editor/SceneEditor.h"
#include "Game/GameScene.h"
#include "Game/GameTime.h"
#include "Game/Nodes/MeshNode.h"
#include "Game/Utility/SceneCreationUtils.h"
#include "Renderer/Material.h"
#include "Renderer/Utility/VkLoader.h"
#include "Renderer/VkTypes.h"

#include "ThirdParty/ImGUI.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Game
{
    GameMain::GameMain(Renderer::VulkanEngine& engine, CVars cvars)
    {
        Renderer::ImageHandle draw_image =
            engine.CreateDrawImage((uint32_t)cvars.width, (uint32_t)cvars.height);
        Renderer::ImageHandle depth_image =
            engine.CreateDepthImage((uint32_t)cvars.width, (uint32_t)cvars.height);

        Renderer::Scene& scene = engine.render_scenes.emplace_back();
        scene.scene_name = "main game scene";
        scene.draw_image = draw_image;
        scene.depth_image = depth_image;

        engine.main_scene = 1;

        m_main_scene = std::make_unique<GameScene>(engine, &scene);
        m_main_editor = std::make_unique<Editor::SceneEditor>(*m_main_scene);

        MainSceneSetup();
    }

    void GameMain::MainSceneSetup()
    {
        Utils::LoadGltfIntoGameScene(m_main_scene->Root(), "../data/resources/BarramundiFish.glb");
    }

    void GameMain::Draw(double delta_time_seconds)
    {
        m_game_time.delta_time_seconds = (float)delta_time_seconds;
        m_game_time.game_time_seconds += m_game_time.delta_time_seconds;

        m_main_scene->Draw(m_game_time);
    }

    void GameMain::OnImGui()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Scene Editor"))
            {
                ImGui::Checkbox("Enable", &m_editor_enabled);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (m_editor_enabled)
        {
            m_main_editor->DrawImGui();
        }
    }
} // namespace Game