#pragma once

#include "Game/ECS.h"

namespace Game::ECS
{
    class GameSystem
    {
    public:
        GameSystem(World& world) : m_world(&world) {}
        virtual ~GameSystem() = default;

        virtual void OnInitialise() {};
        virtual void OnSceneUpdate() {};

    protected:
        World* m_world;
    };
}