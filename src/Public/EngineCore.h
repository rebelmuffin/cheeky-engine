#pragma once

#include "CVars.h"
#include "Renderer/VkEngine.h"

struct SDL_Window;

/// Class that contains the main loop of the engine.
/// This class directly owns the main SDL window and is also responsible for
/// polling events.
class EngineCore
{
  public:
    EngineCore(CVars cvars);
    ~EngineCore();

    void RunMainLoop();
    bool InitialisationFailed();

  private:
    void Update();
    void OnImgui();

    SDL_Window* m_window;
    std::unique_ptr<Renderer::VulkanEngine> m_renderer;

    double m_last_delta_ms = 0;
    int64_t m_last_update_us = 0;
    bool m_initialisation_failure = false;

    bool m_show_imgui_demo = false;
    bool m_show_fps = true;
    bool m_show_compute_effects = false;
};