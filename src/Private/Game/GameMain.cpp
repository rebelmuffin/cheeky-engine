#include "Game/GameMain.h"
#include "Game/GameScene.h"
#include "Game/GameTime.h"
#include "Renderer/VkTypes.h"

namespace Game
{
    GameMain::GameMain(Renderer::VulkanEngine& engine, CVars cvars) : m_main_scene(engine, nullptr)
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

        MainSceneSetup();
    }

    void GameMain::MainSceneSetup() {}

    void GameMain::Draw(double delta_time_seconds)
    {
        m_game_time.delta_time_seconds = (float)delta_time_seconds;
        m_game_time.game_time_seconds += m_game_time.delta_time_seconds;

        m_main_scene.Draw(m_game_time);
    }
} // namespace Game