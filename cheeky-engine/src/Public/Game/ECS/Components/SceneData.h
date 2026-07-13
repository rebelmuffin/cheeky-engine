#pragma once

namespace Physics
{
    class PhysicsScene;
}
namespace Game
{
    class GameScene;
}

namespace Game::ECS
{
    struct SceneData
    {
        GameScene* game_scene = nullptr;
        Physics::PhysicsScene* physics_scene = nullptr;
    };
} // namespace Game::ECS