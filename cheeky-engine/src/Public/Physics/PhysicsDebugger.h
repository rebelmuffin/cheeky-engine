#pragma once

#include "PhysicsScene.h"

namespace Physics
{
    class PhysicsDebugger
    {
    public:
        PhysicsDebugger(PhysicsScene& scene);
        ~PhysicsDebugger() = default;

        void ImGui();

    private:
        PhysicsScene* m_scene;
        bool m_enabled = false;

        bool m_pause_on_contact = false;
    };
}