#pragma once

#include "Game/NodeId.h"

namespace Game::ECS
{
    /* Component that associates an entity with a scene node */
    struct SceneNodeComponent
    {
        NodeId_t node_id{};
    };
}