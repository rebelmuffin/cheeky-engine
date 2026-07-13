#pragma once

#include "Game/ECS/GameSystem.h"

namespace Game::ECS::Systems
{
    class PhysicsUpkeep : public GameSystem
    {
      public:
        PhysicsUpkeep(World& world) : GameSystem(world) {}

        void OnInitialise() override;
    };
} // namespace Game::ECS::Systems