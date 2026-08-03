#pragma once

#include "Colour.h"
#include "Debug/DebuggerBase.h"
#include "glm/vec3.hpp"

namespace Debug
{
    class LineDrawer;

    struct DebugCircle
    {
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        float radius = 1.0f;
        glm::vec3 rotation_euler{};
        size_t segments = 8;
        Cheeky::Colour colour{ 0xFFFFFFFF };
        bool depth = false;
    };

    class LineDrawerDebugger : public DebuggerBase
    {
      public:
        static inline const char* DEBUGGER_NAME = "Line Drawer Debugger";

        explicit LineDrawerDebugger(LineDrawer& line_drawer);
        ~LineDrawerDebugger() override;

        void ImGui(bool& enabled) override;

      private:
        LineDrawer* m_line_drawer = nullptr;

        bool m_draw_circle = false;
        DebugCircle m_circle{};
    };
} // namespace Debug