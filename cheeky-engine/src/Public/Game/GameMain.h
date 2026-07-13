#pragma once

#include "CVars.h"
#include "Game/ECS.h"
#include "Game/ECS/GameSystem.h"
#include "Game/Editor/SceneEditor.h"
#include "Game/GameScene.h"
#include "Game/GameTime.h"
#include "Physics/PhysicsScene.h"
#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"

#include <memory>

namespace Game
{
    /// Container for an entire game instance.
    class GameMain
    {
      public:
        GameMain(Renderer::Window* window, Renderer::VulkanEngine& engine, CVars cvars);
        GameMain(const GameMain&) = delete; // no copy

        void InitECS();
        void MainSceneSetup();

        void Draw(double delta_time_seconds);
        void TickECS();
        void OnImGui();

      private:
        std::unique_ptr<Editor::SceneEditor> m_main_editor{};
        std::unique_ptr<GameScene> m_main_scene;
        std::unique_ptr<Physics::PhysicsScene> m_physics_scene;
        std::unique_ptr<ECS::World> m_ecs_world;
        Renderer::Viewport* m_main_viewport;
        Renderer::VulkanEngine* m_renderer;
        Renderer::Window* m_main_window;
        GameTime m_game_time{};
        CVars m_cvars{};

        std::vector<std::unique_ptr<Game::ECS::GameSystem>> m_systems{};

        bool m_editor_enabled = false;
    };
} // namespace Game