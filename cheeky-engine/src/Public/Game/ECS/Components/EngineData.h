#pragma once

#include "Renderer/VkEngine.h"


namespace Game::ECS
{
    struct EngineData
    {
        Renderer::VulkanEngine* renderer;
    };
}