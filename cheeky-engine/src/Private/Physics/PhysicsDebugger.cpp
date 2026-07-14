#include "Physics/PhysicsDebugger.h"

#include "Utilities/LineDrawer.h"
#include "imgui.h"

namespace
{
    void ImGuiShape(Physics::ShapeId shape_id)
    {
        ImGui::PushID(shape_id.index1);

        ImGui::Text("Name: %s", b3Shape_GetName(shape_id));
        b3AABB aabb = b3Shape_GetAABB(shape_id);
        b3Vec3 center = b3AABB_Center(aabb);
        ImGui::Text("Center: %.2f %.2f %.2f", center.x, center.y, center.z);

        b3ShapeType type = b3Shape_GetType(shape_id);
        if (type == b3_sphereShape)
        {
            b3Sphere sphere = b3Shape_GetSphere(shape_id);
            ImGui::Text("Sphere");
            ImGui::Text("Radius: %.2f", sphere.radius);
        }

        ImGui::PopID();
    }

    void ImGuiBody(Physics::BodyId body_id)
    {
        ImGui::PushID(body_id.index1);

        b3Transform xform = b3Body_GetTransform(body_id);

        ImGui::Text("Name: %s", b3Body_GetName(body_id));
        ImGui::Text("Position: %.2f %.2f %.2f", xform.p.x, xform.p.y, xform.p.z);

        int shape_count = b3Body_GetShapeCount(body_id);
        std::vector<b3ShapeId> shape_ids(shape_count);
        b3Body_GetShapes(body_id, shape_ids.data(), shape_ids.size());
        if (ImGui::TreeNode("body_shapes", "Shapes: %d", shape_count))
        {
            for (int i = 0; i < shape_count; i++)
            {
                ImGuiShape(shape_ids[i]);
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
} // namespace

namespace Physics
{
    PhysicsDebugger::PhysicsDebugger(PhysicsScene& scene) :
        m_scene(&scene),
        m_drawer(scene)
    {
    }

    void PhysicsDebugger::ImGui()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Game"))
            {
                ImGui::Checkbox("Physics Debugger", &m_enabled);
                ImGui::Checkbox("Draw Physics", &m_debug_draw);
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
                        BodyId body_a = b3Shape_GetBody(beginTouchEvent.shapeIdA);
                        ImGuiBody(body_a);

                        BodyId body_b = b3Shape_GetBody(beginTouchEvent.shapeIdB);
                        ImGuiBody(body_b);
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

    void PhysicsDebugger::Draw()
    {
        if (m_debug_draw)
        {
            m_drawer.Draw();
        }
    }
} // namespace Physics