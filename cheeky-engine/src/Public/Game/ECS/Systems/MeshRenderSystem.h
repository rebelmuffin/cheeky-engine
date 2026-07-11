#pragma once

#include "Game/ECS/GameSystem.h"

namespace Game::ECS::Systems
{
    class MeshRenderSystem : public GameSystem
    {
      public:
        MeshRenderSystem(World& world);

        void OnInitialise() override;
    };
} // namespace Game::ECS::Systems