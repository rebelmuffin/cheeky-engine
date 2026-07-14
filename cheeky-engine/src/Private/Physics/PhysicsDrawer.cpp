#include "Physics/PhysicsDrawer.h"

namespace
{
    void DrawSegmentFcn(b3Pos p1, b3Pos p2, b3HexColor, void*)
    {
        glm::vec3 p1v(p1.x, p1.y, p1.z);
        glm::vec3 p2v(p2.x, p2.y, p2.z);

        Debug::LineDrawer::Instance().AddLine(p1v, p2v);
    }
} // namespace

namespace Physics
{
    PhysicsDrawer::PhysicsDrawer(PhysicsScene& scene) :
        m_scene(&scene)
    {
        m_draw_params = b3DefaultDebugDraw();
        m_draw_params.DrawSegmentFcn = &DrawSegmentFcn;
        m_draw_params.drawContacts = true;
        m_draw_params.drawContactNormals = true;
        m_draw_params.drawShapes = true;
    }

    void PhysicsDrawer::Draw()
    {
        b3World_Draw(m_scene->m_world, &m_draw_params, std::numeric_limits<uint64_t>::max());
    }
} // namespace Physics