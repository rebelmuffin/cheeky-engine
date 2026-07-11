#pragma once

namespace Game
{
    class GameScene;
}

namespace Game::ECS
{
    struct SceneData
    {
        GameScene* game_scene = nullptr;
    };
}