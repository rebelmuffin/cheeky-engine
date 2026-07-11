#pragma once
#include "Renderer/FrameDrawContext.h"

namespace Game::ECS
{
    struct RenderContext
    {
        Renderer::FrameDrawContext* draw_context{};
    };
}