#include "EngineCore.h"

#include <SDL.h>

#include <chrono>

EngineCore::EngineCore(uint32_t width, uint32_t height)
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    m_window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                                window_flags);

    m_renderer = std::make_unique<VulkanEngine>(width, height, m_window, /* use_validation_layers = */ true);
    m_renderer->Init();
}

EngineCore::~EngineCore()
{
    m_renderer->Cleanup();
    SDL_DestroyWindow(m_window);
}

void EngineCore::Update()
{
}

void EngineCore::RunMainLoop()
{
    SDL_Event e;
    bool quit = false;
    uint64_t now_us =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock().now().time_since_epoch())
            .count();
    double delta_ms = static_cast<double>(now_us - m_last_update_us) / 1000.0;
    m_last_update_us = now_us;

    while (quit == false)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }

            if (e.type == SDL_WINDOWEVENT)
            {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
                {
                    m_renderer->stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED)
                {
                    m_renderer->stop_rendering = false;
                }
            }
        }

        m_renderer->Update(delta_ms);
    }
}
