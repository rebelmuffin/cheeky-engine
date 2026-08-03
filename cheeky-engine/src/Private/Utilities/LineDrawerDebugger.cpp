#include "Utilities/LineDrawerDebugger.h"

#include "ThirdParty/ImGUI.h"
#include "Utilities/LineDrawer.h"
#include "glm/gtx/euler_angles.hpp"

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
                    ImGui::DragFloat3("Origin", &m_circle.origin.x, 0.1f);
                    ImGui::SliderAngle("Rot X", &m_circle.rotation_euler.x);
                    ImGui::SliderAngle("Rot Y", &m_circle.rotation_euler.x);
                    ImGui::SliderAngle("Rot Z", &m_circle.rotation_euler.x);
                    ImGui::DragFloat("Radius", &m_circle.radius, 0.1f);

                    int segments = m_circle.segments;
                    ImGui::DragInt("Segments", &segments, 1, 2, 256);
                    m_circle.segments = segments;

                    glm::vec4 colour = m_circle.colour.ToVec4();
                    ImGui::ColorEdit4("Colour", &colour.x);
                    m_circle.colour = Cheeky::Colour::FromVec4(colour);

                    ImGui::Checkbox("Depth", &m_circle.depth);
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
        }
        ImGui::End();
    }

} // namespace Debug