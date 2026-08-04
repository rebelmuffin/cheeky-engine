#include "Utilities/LineDrawerDebugger.h"

#include "ThirdParty/ImGUI.h"
#include "Utilities/LineDrawer.h"
#include "glm/gtx/euler_angles.hpp"

namespace
{
    void RenderCircle(Debug::DebugCircle& circle, bool enable_rotation)
    {
        ImGui::DragFloat3("Origin", &circle.origin.x, 0.1f);
        if (enable_rotation)
        {
            ImGui::SliderAngle("Rot X", &circle.rotation_euler.x);
            ImGui::SliderAngle("Rot Y", &circle.rotation_euler.x);
            ImGui::SliderAngle("Rot Z", &circle.rotation_euler.x);
        }
        ImGui::DragFloat("Radius", &circle.radius, 0.1f);

        int segments = circle.segments;
        ImGui::DragInt("Segments", &segments, 1, 2, 256);
        circle.segments = segments;

        glm::vec4 colour = circle.colour.ToVec4();
        ImGui::ColorEdit4("Colour", &colour.x);
        circle.colour = Cheeky::Colour::FromVec4(colour);

        ImGui::Checkbox("Depth", &circle.depth);
    }
} // namespace

namespace Debug
{
    LineDrawerDebugger::LineDrawerDebugger(LineDrawer& line_drawer) :
        DebuggerBase(DEBUGGER_NAME, DebuggerCategory::Core, false),
        m_line_drawer(&line_drawer)
    {
    }
    LineDrawerDebugger::~LineDrawerDebugger() {}

    void LineDrawerDebugger::ImGui(bool& enabled)
    {
        if (ImGui::Begin(m_name.data(), &enabled))
        {
            ImGui::Checkbox("Draw Circle", &m_draw_circle);
            if (m_draw_circle)
            {
                if (ImGui::CollapsingHeader("Circle Settings"))
                {
                    RenderCircle(m_circle, true);
                }

                m_line_drawer->AddCircle(
                    m_circle.origin,
                    m_circle.radius,
                    glm::eulerAngleXYZ(
                        m_circle.rotation_euler.x, m_circle.rotation_euler.y, m_circle.rotation_euler.z
                    ),
                    m_circle.segments,
                    ONE_FRAME,
                    m_circle.colour,
                    m_circle.depth
                );
            }

            ImGui::Checkbox("Draw Sphere", &m_draw_sphere);
            if (m_draw_sphere)
            {
                if (ImGui::CollapsingHeader("Sphere Settings"))
                {
                    RenderCircle(m_sphere, false);
                }

                m_line_drawer->AddSphere(
                    m_sphere.origin,
                    m_sphere.radius,
                    m_sphere.segments,
                    ONE_FRAME,
                    m_sphere.colour,
                    m_sphere.depth
                );
            }
        }
        ImGui::End();
    }

} // namespace Debug