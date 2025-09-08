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
    GameMain::GameMain(Renderer::VulkanEngine& engine, CVars cvars) :
        m_main_viewport(&engine.active_viewports[engine.main_viewport]),
        m_renderer(&engine),
        m_cvars(cvars)
    {
        m_main_scene = std::make_unique<GameScene>();
        m_main_editor = std::make_unique<Editor::SceneEditor>(*m_main_scene);

        MainSceneSetup();
    }

    void GameMain::MainSceneSetup()
    {
        Utils::LoadGltfIntoGameScene(*m_renderer, m_main_scene->Root(), m_cvars.default_scene_path);
    }

    void GameMain::Draw(double delta_time_seconds)
    {
        m_game_time.delta_time_seconds = (float)delta_time_seconds;
        m_game_time.game_time_seconds += m_game_time.delta_time_seconds;

        // draw on the main viewport.
        m_main_scene->Draw(m_main_viewport->frame_context);
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