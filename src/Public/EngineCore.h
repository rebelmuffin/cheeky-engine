#pragma once

#include "VkEngine.h"

struct SDL_Window;

/// Class that contains the main loop of the engine.
/// This class directly owns the main SDL window and is also responsible for
/// polling events.
class EngineCore
{
  public:
    EngineCore(int width, int height);
    ~EngineCore();

    void RunMainLoop();
    bool InitialisationFailed();

  private:
    void Update();

    SDL_Window* m_window;
    std::unique_ptr<VulkanEngine> m_renderer;

    int64_t m_last_update_us = 0;
    bool m_initialisation_failure = false;
};