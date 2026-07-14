#pragma once

#include "PhysicsScene.h"
#include "Utilities/LineDrawer.h"

namespace Physics
{
    class PhysicsDrawer
    {
    public:
        PhysicsDrawer(PhysicsScene& scene);
        ~PhysicsDrawer() = default;

        void Draw();

    private:
        PhysicsScene* m_scene;
        b3DebugDraw m_draw_params{};

        friend class PhysicsDebugger;
    };
}