#pragma once

#include "CVars.h"
#include "Game/Editor/SceneEditor.h"
#include "Game/GameScene.h"
#include "Game/GameTime.h"
#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"

#include <memory>

namespace Game
{
    /// Container for an entire game instance.
    class GameMain
    {
      public:
        GameMain(Renderer::VulkanEngine& engine, CVars cvars);
        GameMain(const GameMain&) = delete; // no copy

        void MainSceneSetup();
        void Draw(double delta_time_seconds);
        void OnImGui();

      private:
        std::unique_ptr<Editor::SceneEditor> m_main_editor{};
        std::unique_ptr<GameScene> m_main_scene;
        Renderer::Viewport* m_main_render_scene;
        Renderer::VulkanEngine* m_renderer;
        GameTime m_game_time{};
        CVars m_cvars{};

        bool m_editor_enabled = true;
    };
} // namespace Game