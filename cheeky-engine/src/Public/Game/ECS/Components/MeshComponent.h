#pragma once

#include "Renderer/Utility/VkLoader.h"

namespace Game::ECS
{
    struct MeshComponent
    {
        Renderer::MeshHandle mesh_asset;
    };
}