#pragma once

#include "VkTypes.h"

struct SDL_Window;

class VulkanEngine
{
  public:
    VulkanEngine(uint32_t window_width, uint32_t window_height, SDL_Window* window);

    bool is_initialised{false};
    int frame_number{0};
    bool stop_rendering{false};

    // initializes everything in the engine
    void Init();

    // shuts down the engine
    void Cleanup();

    // run main loop
    void Update(double delta_ms);

  private:
    // draw loop
    void Draw(double delta_ms);

    VkExtent2D m_window_extent;
    SDL_Window* m_window;

    uint64_t m_last_update_us = 0;
};