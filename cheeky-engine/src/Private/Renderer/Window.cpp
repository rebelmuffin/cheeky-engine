#include "Renderer/Window.h"

#include <SDL3/SDL.h>

namespace Renderer
{
    Window::Window(const int initial_width, const int initial_height, const int pos_x, const int pos_y)
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        m_window = SDL_CreateWindow("Cheeky", initial_width, initial_height, window_flags);
        SDL_SetWindowPosition(m_window, pos_x, pos_y);
        m_start_width = initial_width;
        m_start_height = initial_height;
    }

    Window::~Window() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }

    void Window::Resize(const int width, const int height)
    {
        int cur_w, cur_h;
        SDL_GetWindowSize(m_window, &cur_w, &cur_h);
        if (cur_w != width || cur_h != height)
        {
            SDL_SetWindowSize(m_window, width, height);
        }
    }

    void Window::SetCaptureMouse([[maybe_unused]] const bool capture_mouse)
    {
#ifndef __APPLE__
        SDL_SetWindowRelativeMouseMode(m_window, capture_mouse);
#endif
    }

    SDL_Surface* Window::GetSurface() const { return SDL_GetWindowSurface(m_window); }

    bool Window::IsPixelPositionInWindow(const int x, const int y) const
    {
        int window_x, window_y;
        int window_w, window_h;
        SDL_GetWindowPosition(m_window, &window_x, &window_y);
        SDL_GetWindowSize(m_window, &window_w, &window_h);

        int max_x = window_x + window_w;
        int max_y = window_y + window_h;
        return x > window_x && x <= max_x && y > window_y && y <= max_y;
    }

    void Window::GetDimensions(int* width, int* height) const { SDL_GetWindowSize(m_window, width, height); }

} // namespace Renderer