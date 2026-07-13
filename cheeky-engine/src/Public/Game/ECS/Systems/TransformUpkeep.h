#pragma once

#include "Game/ECS/GameSystem.h"

namespace Game::ECS::Systems
{
    class TransformUpkeep : public GameSystem
    {
      public:
        TransformUpkeep(World& world);

        void OnInitialise() override;
    };
} // namespace Game::ECS::Systems
