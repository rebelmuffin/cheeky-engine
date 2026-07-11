#pragma once

#include "Game/ECS/Components/TransformComponent.h"
#include "Game/ECS/GameSystem.h"
#include "Game/Node.h"

namespace Game::ECS::Systems
{
    class TransformUpkeep : public GameSystem
    {
      public:
        TransformUpkeep(World& world);

        void OnInitialise() override;
    };
} // namespace Game::ECS::Systems
