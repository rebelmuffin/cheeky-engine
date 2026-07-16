#pragma once

#include "DrawDuration.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace Renderer
{
    struct GLTFMaterial;
    class VulkanEngine;
    struct Viewport;
} // namespace Renderer

namespace Debug
{
    using Colour = glm::vec4;
    struct DebugLine
    {
        DrawDuration duration{};
        glm::vec3 start{}, end{};
        Colour colour = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool z_depth = false;
    };
    constexpr Colour WHITE = Colour(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr Colour BLACK = Colour(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr Colour RED = Colour(1.0f, 0.0f, 0.0f, 1.0f);
    constexpr Colour GREEN = Colour(0.0f, 1.0f, 0.0f, 1.0f);
    constexpr Colour BLUE = Colour(0.0f, 0.0f, 1.0f, 1.0f);

    class LineDrawer
    {
      public:
        LineDrawer() = default;
        ~LineDrawer() = default;

        static LineDrawer& Instance();

        void AddLine(
            glm::vec3 start,
            glm::vec3 end,
            DrawDuration duration = ONE_FRAME,
            Colour colour = WHITE,
            bool z_depth = true
        );

        void OnRender(Renderer::VulkanEngine& renderer, Renderer::Viewport& viewport, double time_delta_s);

      private:
        void UpdateLineDurations(double time_delta_s);
        static std::shared_ptr<Renderer::GLTFMaterial> CreateMaterial(Renderer::VulkanEngine& renderer, bool depth);

        std::vector<DebugLine> m_lines{};
        std::shared_ptr<Renderer::GLTFMaterial> m_depth_material{};
        std::shared_ptr<Renderer::GLTFMaterial> m_no_depth_material{};

        static std::unique_ptr<LineDrawer> s_line_drawer;
    };
} // namespace Debug