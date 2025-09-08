#include "EngineCore.h"
#include "CVars.h"
#include "Game/GameMain.h"

#include "ImGuizmo.h"
#include "ThirdParty/ImGUI.h"
#include <SDL.h>

#include <chrono>
#include <memory>

EngineCore::EngineCore(CVars cvars)
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    m_window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        cvars.width,
        cvars.height,
        window_flags
    );

    m_renderer = std::make_unique<Renderer::VulkanEngine>(
        cvars.width,
        cvars.height,
        m_window,
        cvars.backbuffer_scale,
        cvars.use_validation_layers,
        cvars.force_immediate_uploads
    );
    if (m_renderer->Init() == false)
    {
        m_initialisation_failure = true;
    }

    // load imgui fonts
    constexpr const char* font_path = "../data/fonts/roboto.ttf";
    ImGui::GetIO().Fonts->AddFontFromFileTTF(font_path, 14);

    m_game = std::make_unique<Game::GameMain>(*m_renderer, cvars);
}

EngineCore::~EngineCore()
{
    m_renderer->Cleanup();
    SDL_DestroyWindow(m_window);
}

void EngineCore::Update() {}

void EngineCore::RunMainLoop()
{
    SDL_Event e;
    bool quit = false;

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

            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        // update delta time
        int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::steady_clock().now().time_since_epoch()
        )
                             .count();
        m_last_delta_ms = static_cast<double>(now_us - m_last_update_us) / 1000.0;
        m_last_update_us = now_us;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        // ImGuizmo::BeginFrame();

        OnImgui();

        m_game->Draw(m_last_delta_ms / 1000.0);

        // renderer draw should be after any other kind of draw because things "queue" render objects for the
        // renderer to render during its draw.
        m_renderer->Update();

        // any logical updates
        Update();
    }
}

void EngineCore::OnImgui()
{
    float imgui_menu_cursor_y = 0;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("ImGUI"))
        {
            ImGui::Checkbox("Show Demo", &m_show_imgui_demo);
            ImGui::Checkbox("Frame Stats", &m_show_fps);
            ImGui::EndMenu();
        }

        imgui_menu_cursor_y = ImGui::GetTextLineHeightWithSpacing() * 2.0f;
        ImGui::EndMainMenuBar();
    }

    if (m_show_imgui_demo)
    {
        ImGui::ShowDemoWindow(&m_show_imgui_demo);
    }

    if (m_show_fps)
    {
        // draw time delta and FPS
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        double delta_s = m_last_delta_ms / 1000.0;
        std::ostringstream stream;
        stream << "FPS: " << std::setw(5) << int64_t(1.0 / delta_s) << " | " << std::fixed
               << std::setprecision(2) << m_last_delta_ms << "ms";
        std::string fps_text = stream.str();
        draw_list->AddText(
            { 0.0f, imgui_menu_cursor_y },
            ImGui::GetColorU32(ImGuiCol_Text),
            fps_text.data(),
            fps_text.data() + fps_text.size()
        );
    }

    m_game->OnImGui();
}

bool EngineCore::InitialisationFailed() { return m_initialisation_failure; }
