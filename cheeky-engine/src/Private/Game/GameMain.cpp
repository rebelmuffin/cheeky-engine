#include "Game/GameMain.h"
#include "Game/ECS/Components/EngineData.h"
#include "Game/ECS/Components/RenderContext.h"
#include "Game/ECS/Components/SceneData.h"
#include "Game/ECS/Components/TransformComponent.h"
#include "Game/ECS/GameSystem.h"
#include "Game/ECS/Systems/MeshRenderSystem.h"
#include "Game/ECS/Systems/PhysicsSimulationSystem.h"
#include "Game/ECS/Systems/PhysicsUpkeep.h"
#include "Game/ECS/Systems/TransformUpkeep.h"
#include "Game/Editor/SceneEditor.h"
#include "Game/GameScene.h"
#include "Game/GameTime.h"
#include "Game/Utility/SceneCreationUtils.h"
#include "Renderer/Utility/Camera.h"
#include "Renderer/Utility/VkLoader.h"

#include "ThirdParty/ImGUI.h"

#include <memory>
#include <vector>

namespace Game
{
    GameMain::GameMain(Renderer::Window* window, Renderer::VulkanEngine& engine, CVars cvars) :
        m_main_viewport(&engine.active_viewports[engine.main_viewport]),
        m_renderer(&engine),
        m_main_window(window),
        m_cvars(cvars)
    {
        m_main_scene = std::make_unique<GameScene>();
        m_main_editor = std::make_unique<Editor::SceneEditor>(*m_main_scene);
        m_physics_scene = std::make_unique<Physics::PhysicsScene>();
        m_physics_debugger = std::make_unique<Physics::PhysicsDebugger>(*m_physics_scene);

        InitECS();
        MainSceneSetup();
    }

    void GameMain::InitECS()
    {
        m_ecs_world = std::make_unique<ECS::World>();
        m_ecs_world->set<ECS::SceneData>({ m_main_scene.get(), m_physics_scene.get() });
        m_ecs_world->set<ECS::EngineData>({ m_renderer });

        m_systems.emplace_back(std::make_unique<ECS::Systems::TransformUpkeep>(*m_ecs_world));
        m_systems.emplace_back(std::make_unique<ECS::Systems::PhysicsUpkeep>(*m_ecs_world));
        m_systems.emplace_back(std::make_unique<ECS::Systems::PhysicsSimulationSystem>(*m_ecs_world));
        m_systems.emplace_back(std::make_unique<ECS::Systems::MeshRenderSystem>(*m_ecs_world));

        for (const std::unique_ptr<ECS::GameSystem>& system : m_systems)
        {
            system->OnInitialise();
        }
    }

    void GameMain::MainSceneSetup()
    {
        Utils::LoadGltfIntoGameSceneECS(*m_ecs_world, m_main_scene->Root(), m_cvars.default_scene_path);
    }

    void GameMain::Draw(double delta_time_seconds)
    {
        m_game_time.delta_time_seconds = (float)delta_time_seconds;
        m_game_time.game_time_seconds += m_game_time.delta_time_seconds;

        // draw on the main viewport.
        m_main_editor->Draw(delta_time_seconds, m_main_window, *m_main_viewport);

        Renderer::Camera* override_camera = nullptr;
        if (m_main_editor->EditorCameraEnabled())
        {
            override_camera = &m_main_editor->Camera();
        }

        m_main_scene->Draw(*m_main_viewport, override_camera);

        TickECS();
    }

    void GameMain::TickECS()
    {
        m_ecs_world->set<GameTime>(m_game_time);
        m_ecs_world->set<ECS::RenderContext>({ &m_main_viewport->frame_context });
        std::ignore = m_ecs_world->progress(m_game_time.delta_time_seconds);
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

        m_physics_debugger->ImGui();
    }
} // namespace Game