#include "Physics/PhysicsDebugger.h"

#include "imgui.h"

namespace Physics
{
    PhysicsDebugger::PhysicsDebugger(PhysicsScene& scene) : m_scene(&scene) {}

    void PhysicsDebugger::ImGui()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Game"))
            {
                ImGui::Checkbox("Physics Debugger", &m_enabled);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (m_enabled == false)
        {
            return;
        }

        if (ImGui::Begin("Physics Debugger", &m_enabled))
        {
            ImGui::Checkbox("Paused", &m_scene->m_paused);
            ImGui::InputFloat("Update Frequency", &m_scene->m_frequency);
            if (ImGui::Button("Step Once"))
            {
                bool previous_pause = m_scene->m_paused;
                m_scene->m_paused = false;
                m_scene->Step();
                m_scene->m_paused = previous_pause;
            }

            b3ContactEvents contacts = b3World_GetContactEvents(m_scene->m_world);
            if (ImGui::CollapsingHeader("Contacts"))
            {
                ImGui::Checkbox("Pause On Contact", &m_pause_on_contact);

                if (contacts.beginCount > 0 && m_pause_on_contact)
                {
                    m_pause_on_contact = false;
                    m_scene->m_paused = true;
                }
                if (ImGui::TreeNode("contacts_begin", "Contacts Begin: %d", contacts.beginCount))
                {
                    for (int i = 0; i < contacts.beginCount; i++)
                    {
                        const b3ContactBeginTouchEvent& beginTouchEvent = contacts.beginEvents[i];
                        ImGui::Text("Shape A: %s", b3Shape_GetName(beginTouchEvent.shapeIdA));
                        ImGui::Text("Shape B: %s", b3Shape_GetName(beginTouchEvent.shapeIdB));
                    }
                    ImGui::TreePop();
                }
            }

            b3BodyEvents evts = b3World_GetBodyEvents(m_scene->m_world);
            if (ImGui::TreeNode("move_events", "Last Move Events: %d", evts.moveCount))
            {
                for (int i = 0; i < evts.moveCount; i++)
                {
                    const b3BodyMoveEvent& moveEvent = evts.moveEvents[i];
                    ImGui::Text("Body: %s", b3Body_GetName(moveEvent.bodyId));
                    ImGui::Text(
                        "Position: %.2f %.2f %.2f",
                        moveEvent.transform.p.x,
                        moveEvent.transform.p.y,
                        moveEvent.transform.p.z
                    );
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }
} // namespace Physics