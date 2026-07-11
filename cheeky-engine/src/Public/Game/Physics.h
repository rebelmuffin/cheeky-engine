#pragma once

#include "box3d/box3d.h"

namespace Game
{
    struct Transform;
}
namespace Game::Physics
{


    PWorld CreateWorld();
    Body CreateBody(PWorld world, const Transform& world_transform, bool dynamic = true);
    void StepWorld(PWorld world, float time_step);
}