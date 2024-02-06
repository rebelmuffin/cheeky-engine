#pragma once

#include "VkEngine.h"

struct SDL_Window;

/// Class that contains the main loop of the engine.
/// This class directly owns the main SDL window and is also responsible for
/// polling events.
class EngineCore
{
  public:
    EngineCore(uint32_t width, uint32_t height);
    ~EngineCore();

    void RunMainLoop();

  private:
    void Update();

    SDL_Window* m_window;
    std::unique_ptr<VulkanEngine> m_renderer;

    uint64_t m_last_update_us = 0;
};