#include "EngineCore.h"
#include "CVars.h"
#include "Game/GameMain.h"

#include "ImGuizmo.h"
#include "ThirdParty/ImGUI.h"
#include "Utilities/LineDrawer.h"

#include <SDL3/SDL.h>

#include <chrono>
#include <memory>

EngineCore::EngineCore(CVars cvars) : m_cvars(cvars)
{
    m_window = std::make_unique<Renderer::Window>(
        cvars.width, cvars.height, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED
    );

    m_renderer = std::make_unique<Renderer::VulkanEngine>(
        *m_window, cvars.backbuffer_scale, cvars.use_validation_layers, cvars.force_immediate_uploads
    );
    if (m_renderer->Init() == false)
    {
        m_initialisation_failure = true;
        return;
    }

    // load imgui fonts
    constexpr const char* font_path = "data/fonts/roboto.ttf";
    ImGui::GetIO().Fonts->AddFontFromFileTTF(font_path, 14);

    m_game = std::make_unique<Game::GameMain>(m_window.get(), *m_renderer, cvars);
    m_last_update_us = std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::steady_clock().now().time_since_epoch()
    )
                           .count();
}

EngineCore::~EngineCore() { m_renderer->Cleanup(); }

void EngineCore::Update() {}

void EngineCore::RunMainLoop()
{
    SDL_Event e;
    bool quit = false;

    while (quit == false)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }

            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                m_renderer->stop_rendering = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED)
            {
                m_renderer->stop_rendering = false;
            }

            ImGui_ImplSDL3_ProcessEvent(&e);
        }

        // update delta time
        int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::steady_clock().now().time_since_epoch()
        )
                             .count();
        m_last_delta_ms = static_cast<double>(now_us - m_last_update_us) / 1000.0;
        m_last_update_us = now_us;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        OnImgui();

        m_game->Draw(m_last_delta_ms / 1000.0);

        Debug::LineDrawer::Instance().AddLine(glm::vec3(100.0f, 0.0f, 0.0f), glm::vec3(-100.0f, 0.0f, 0.0f));
        Debug::LineDrawer::Instance().AddLine(glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, -100.0f, 0.0f));

        Debug::LineDrawer::Instance().OnRender(
            *m_renderer, m_renderer->active_viewports[m_renderer->main_viewport], m_last_delta_ms / 1000.0f
        );

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

        if (ImGui::BeginMenu("Game"))
        {
            if (ImGui::Button("Restart"))
            {
                RestartGame();
            }
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

void EngineCore::RestartGame()
{
    m_game = std::make_unique<Game::GameMain>(m_window.get(), *m_renderer, m_cvars);
}

bool EngineCore::InitialisationFailed() { return m_initialisation_failure; }
