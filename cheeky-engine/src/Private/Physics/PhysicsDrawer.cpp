#include "Physics/PhysicsDrawer.h"

namespace
{
    constexpr bool depth = false;

    Cheeky::Colour b3ColourToCheekyColour(b3HexColor colour, float alpha = 1.0f)
    {
        uint8_t a = alpha * 255.0f;
        return Cheeky::Colour{ static_cast<uint32_t>(colour << 8) + a };
    }

    void DrawSegmentFcn(b3Pos p1, b3Pos p2, b3HexColor colour, void*)
    {
        glm::vec3 p1v(p1.x, p1.y, p1.z);
        glm::vec3 p2v(p2.x, p2.y, p2.z);

        Debug::LineDrawer::Instance().AddLine(
            p1v, p2v, Debug::ONE_FRAME, b3ColourToCheekyColour(colour), depth
        );
    }

    void DrawSphereFcn(b3Pos p, float radius, b3HexColor colour, float alpha, void*)
    {
        glm::vec3 origin(p.x, p.y, p.z);
        Debug::LineDrawer::Instance().AddSphere(
            origin, radius, 16, Debug::ONE_FRAME, b3ColourToCheekyColour(colour, alpha), depth
        );
    }
} // namespace

namespace Physics
{
    PhysicsDrawer::PhysicsDrawer(PhysicsScene& scene) : m_scene(&scene)
    {
        m_draw_params = b3DefaultDebugDraw();
        m_draw_params.DrawSegmentFcn = &DrawSegmentFcn;
        m_draw_params.DrawSphereFcn = &DrawSphereFcn;
        m_draw_params.drawContacts = true;
        m_draw_params.drawContactNormals = true;
        m_draw_params.drawShapes = true;
    }

    void PhysicsDrawer::Draw()
    {
        b3World_Draw(m_scene->m_world, &m_draw_params, std::numeric_limits<uint64_t>::max());
    }
} // namespace Physics