#include "Renderer/Window.h"

#include <SDL.h>

namespace Renderer
{
    Window::Window(int initial_width, int initial_height, int pos_x, int pos_y)
    {
        SDL_InitSubSystem(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        m_window = SDL_CreateWindow("Cheeky", pos_x, pos_y, initial_width, initial_height, window_flags);
        m_start_width = initial_width;
        m_start_height = initial_height;
    }

    Window::~Window() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }

    void Window::Resize(int width, int height)
    {
        int cur_w, cur_h;
        SDL_GetWindowSize(m_window, &cur_w, &cur_h);
        if (cur_w != width || cur_h != height)
        {
            SDL_SetWindowSize(m_window, width, height);
        }
    }

    SDL_Surface* Window::GetSurface() const { return SDL_GetWindowSurface(m_window); }

    void Window::GetDimensions(int* width, int* height) const { SDL_GetWindowSize(m_window, width, height); }

} // namespace Renderer