#pragma once

#include "Game/ECS/GameSystem.h"

namespace Game::ECS::Systems
{
    class PhysicsSimulationSystem : public GameSystem
    {
      public:
        PhysicsSimulationSystem(World& world) : GameSystem(world) {}

        void OnInitialise() override;
    };
} // namespace Game::ECS::Systems