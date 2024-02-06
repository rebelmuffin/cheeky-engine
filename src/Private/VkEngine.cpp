#include "VkEngine.h"

#include <SDL.h>

#include <thread>

VulkanEngine::VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window)
    : m_window_extent({window_width, window_height}), m_window(window)
{
}

void VulkanEngine::Init()
{
    is_initialised = true;
}

void VulkanEngine::Cleanup()
{
    is_initialised = false;
}

void VulkanEngine::Draw(double delta_ms)
{
}

void VulkanEngine::Update(double delta_ms)
{
    SDL_Event e;
    bool quit = false;

    if (stop_rendering)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    Draw(delta_ms);
}
