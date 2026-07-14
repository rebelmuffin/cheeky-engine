#pragma once

#include "PhysicsDrawer.h"
#include "PhysicsScene.h"

namespace Physics
{
    class PhysicsDebugger
    {
      public:
        PhysicsDebugger(PhysicsScene& scene);
        ~PhysicsDebugger() = default;

        void ImGui();
        void Draw();

      private:
        PhysicsScene* m_scene;
        PhysicsDrawer m_drawer;
        bool m_enabled = false;
        bool m_debug_draw = false;

        bool m_pause_on_contact = false;
    };
} // namespace Physics