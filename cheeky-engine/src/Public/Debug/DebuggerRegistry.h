#pragma once

#include "DebuggerBase.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Debug
{
    /*
     * App-wide singleton that stores the active debuggers
     */
    class DebuggerRegistry
    {
      public:
        static DebuggerRegistry& Instance();

        void AddDebugger(DebuggerBase* debugger);
        void RemoveDebugger(std::string_view debugger_name);
        void SetDebuggingEnabled(bool enabled);

        void ImGui();
        void Draw(double delta_time_seconds);
        bool IsDebuggingEnabled() const { return m_enabled; }

      private:
        DebuggerRegistry() = default;
        ~DebuggerRegistry() = default;
        DebuggerRegistry(const DebuggerRegistry&) = delete;
        DebuggerRegistry& operator=(const DebuggerRegistry&) = delete;

        void SetDebuggerEnabled(DebuggerBase& debugger, bool enabled);

        bool m_enabled = false;
        std::vector<bool> m_debugger_enabled_states{};
        std::vector<std::unique_ptr<DebuggerBase>> m_debuggers{};
    };
} // namespace Debug