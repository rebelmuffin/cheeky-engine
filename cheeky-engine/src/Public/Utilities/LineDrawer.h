#pragma once

#include "DrawDuration.h"
#include "Renderer/Utility/VkLoader.h"
#include "Utilities/Colour.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
    constexpr Cheeky::Colour WHITE = Cheeky::Colour(0xFFFFFFFF);
    constexpr Cheeky::Colour BLACK = Cheeky::Colour(0x000000FF);
    constexpr Cheeky::Colour RED = Cheeky::Colour(0xFF0000FF);
    constexpr Cheeky::Colour GREEN = Cheeky::Colour(0x00FF00FF);
    constexpr Cheeky::Colour BLUE = Cheeky::Colour(0x0000FFFF);
    struct DebugLine
    {
        DrawDuration duration{};
        glm::vec3 start{}, end{};
        Cheeky::Colour colour = WHITE;
        bool z_depth = false;
    };

    class LineDrawer
    {
      public:
        LineDrawer();
        ~LineDrawer() = default;

        static LineDrawer& Instance();

        void AddLine(
            glm::vec3 start,
            glm::vec3 end,
            DrawDuration duration = ONE_FRAME,
            Cheeky::Colour colour = WHITE,
            bool z_depth = true
        );
        void AddCircle(
            glm::vec3 origin,
            float radius,
            glm::quat rotation = glm::identity<glm::quat>(),
            size_t segments = 8,
            DrawDuration duration = ONE_FRAME,
            Cheeky::Colour colour = WHITE,
            bool z_depth = false
        );
        void AddSphere(
            glm::vec3 origin,
            float radius,
            size_t segments = 8,
            DrawDuration duration = ONE_FRAME,
            Cheeky::Colour colour = WHITE,
            bool z_depth = false
        );
        // void AddBox(
        //     glm::vec3 origin,
        //     glm::vec3 half_widths,
        //     glm::quat rotation = glm::identity<glm::quat>(),
        //     DrawDuration duration = ONE_FRAME,
        //     Cheeky::Colour colour = WHITE,
        //     bool z_depth = false
        // );
        //
        void OnRender(Renderer::VulkanEngine& renderer, Renderer::Viewport& viewport, double time_delta_s);

      private:
        void UpdateLineDurations(double time_delta_s);
        static std::shared_ptr<Renderer::GLTFMaterial> CreateMaterial(
            Renderer::VulkanEngine& renderer, bool depth
        );

        Renderer::MeshHandle m_draw_mesh{};
        std::vector<DebugLine> m_lines{};
        std::shared_ptr<Renderer::GLTFMaterial> m_depth_material{};
        std::shared_ptr<Renderer::GLTFMaterial> m_no_depth_material{};

        static std::unique_ptr<LineDrawer> s_line_drawer;
    };
} // namespace Debug