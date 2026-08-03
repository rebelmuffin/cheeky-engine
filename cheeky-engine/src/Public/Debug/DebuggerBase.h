#pragma once

#include <string>

namespace Debug
{
    enum class DebuggerCategory
    {
        Core,
        Renderer,
        Physics,
        Game
    };

    /*
     * Class used as the base for all debugging facilities in the engine.
     */
    class DebuggerBase
    {
      public:
        DebuggerBase(std::string_view name, DebuggerCategory category, bool enabled_by_default);
        virtual ~DebuggerBase() = default;

        virtual void OnEnabled() {}
        virtual void OnDisabled() {}

        virtual void ImGui([[maybe_unused]] bool& enabled) {}
        virtual void Draw([[maybe_unused]] double delta_time_seconds) {}

        const std::string& Name() { return m_name; }
        bool IsEnabled() const { return m_enabled; }
        void SetEnabled(const bool enabled) { m_enabled = enabled; }

      protected:
        std::string m_name{};
        DebuggerCategory m_category = DebuggerCategory::Core;
        bool m_enabled = false;
    };
} // namespace Debug