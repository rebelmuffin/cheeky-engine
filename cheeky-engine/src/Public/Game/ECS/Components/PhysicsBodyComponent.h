#pragma once

#include "Physics/PhysicsBody.h"

namespace Game::ECS
{
    struct PhysicsBodyComponent
    {
        bool is_dynamic = true;
    };
}