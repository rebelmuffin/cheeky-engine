#pragma once

#include "CVars.h"
#include "Game/GameScene.h"
#include "Game/GameTime.h"

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

      private:
        GameScene m_main_scene;
        GameTime m_game_time{};
    };
} // namespace Game