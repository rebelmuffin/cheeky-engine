#pragma once

struct SDL_Window;
struct SDL_Surface;

namespace Renderer
{
    class Window
    {
      public:
        Window(int initial_width, int initial_height, int pos_x = 0, int pos_y = 0);
        ~Window();

        void Resize(int width, int height);

        [[nodiscard]] SDL_Window* GetWindow() const { return m_window; }
        [[nodiscard]] SDL_Surface* GetSurface() const;

        void GetDimensions(int* width, int* height) const;

      private:
        int m_start_width{}, m_start_height{};
        SDL_Window* m_window{ nullptr };
    };
} // namespace Renderer
