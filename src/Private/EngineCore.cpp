#include "EngineCore.h"

#include "ThirdParty/ImGUI.h"
#include "imgui.h"

#include <SDL.h>

#include <chrono>

EngineCore::EngineCore(int width, int height)
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    m_window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                                window_flags);

    m_renderer = std::make_unique<VulkanEngine>(width, height, m_window, 1.0f, /* use_validation_layers = */ true);
    if (m_renderer->Init() == false)
    {
        m_initialisation_failure = true;
    }

    // load imgui fonts
    constexpr const char* font_path = "../data/fonts/roboto.ttf";
    ImGui::GetIO().Fonts->AddFontFromFileTTF(font_path, 14);
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
        int64_t now_us =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock().now().time_since_epoch())
                .count();
        m_last_delta_ms = static_cast<double>(now_us - m_last_update_us) / 1000.0;
        m_last_update_us = now_us;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        OnImgui();

        m_renderer->Update(m_last_delta_ms);
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

        if (ImGui::BeginMenu("Graphics"))
        {
            ImGui::Checkbox("Compute Effects", &m_show_compute_effects);
            ImGui::Checkbox("Engine Settings", &m_show_engine_settings);
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
        stream << "FPS: " << std::setw(5) << int64_t(1.0 / delta_s) << " | " << std::fixed << std::setprecision(2)
               << m_last_delta_ms << "ms";
        std::string fps_text = stream.str();
        draw_list->AddText({0.0f, imgui_menu_cursor_y}, ImGui::GetColorU32(ImGuiCol_Text), fps_text.data(),
                           fps_text.data() + fps_text.size());
    }

    if (m_show_compute_effects)
    {
        if (ImGui::Begin("Compute Effects", &m_show_compute_effects))
        {
            auto copy_vec_to_array = +[](const glm::vec4& vec, float* data) {
                data[0] = vec.x;
                data[1] = vec.y;
                data[2] = vec.z;
                data[3] = vec.w;
            };
            auto copy_array_to_vec = +[](const float* data, glm::vec4& vec) {
                vec.x = data[0];
                vec.y = data[1];
                vec.z = data[2];
                vec.w = data[3];
            };

            static int current_selection = 0;
            ComputeEffect* current_effect = &m_renderer->ComputeEffects()[m_renderer->CurrentComputeEffect()];
            PushConstants* constants = &current_effect->push_constants;
            static float data1[4];
            static float data2[4];
            static float data3[4];
            static float data4[4];
            copy_vec_to_array(constants->data1, data1);
            copy_vec_to_array(constants->data2, data2);
            copy_vec_to_array(constants->data3, data3);
            copy_vec_to_array(constants->data4, data4);

            if (ImGui::Combo("Current Effect", &current_selection, "sky\0gradient_color\0"))
            {
                m_renderer->SetCurrentComputeEffect(uint(current_selection));

                current_effect = &m_renderer->ComputeEffects()[uint(current_selection)];
                constants = &current_effect->push_constants;
            }

            ImGui::Text("Shader Path: %s", current_effect->path);
            if (ImGui::ColorEdit4("data1", data1))
            {
                copy_array_to_vec(data1, constants->data1);
            }
            if (ImGui::ColorEdit4("data2", data2))
            {
                copy_array_to_vec(data2, constants->data2);
            }
            if (ImGui::ColorEdit4("data3", data3))
            {
                copy_array_to_vec(data3, constants->data3);
            }
            if (ImGui::ColorEdit4("data4", data4))
            {
                copy_array_to_vec(data4, constants->data4);
            }
        }
        ImGui::End();
    }

    if (m_show_engine_settings)
    {
        if (ImGui::Begin("Engine Settings", &m_show_engine_settings))
        {
            if (ImGui::Button("Reset Swapchain"))
            {
                // m_renderer->ResetSwapchain();
            }
        }
        ImGui::End();
    }
}

bool EngineCore::InitialisationFailed()
{
    return m_initialisation_failure;
}
