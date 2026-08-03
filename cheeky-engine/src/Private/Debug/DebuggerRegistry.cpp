#include "Debug/DebuggerRegistry.h"

#include "Utilities/Log.h"
#include "imgui.h"

namespace Debug
{
    DebuggerRegistry& DebuggerRegistry::Instance()
    {
        static DebuggerRegistry s_instance{};
        return s_instance;
    }

    void DebuggerRegistry::AddDebugger(DebuggerBase* debugger)
    {
        for (size_t i = 0; i < m_debuggers.size(); i++)
        {
            std::unique_ptr<DebuggerBase>& existing = m_debuggers[i];
            if (existing->Name() == debugger->Name())
            {
                LOG_ERROR(
                    "DEBUGGER REGISTRY - Trying to register debugger with name {} more than once!",
                    existing->Name()
                );

                // shitty recovery, we're probably cooked anyway
                existing = std::unique_ptr<DebuggerBase>(debugger);
                m_debugger_enabled_states[i] = debugger->IsEnabled();

                return;
            }
        }

        m_debuggers.emplace_back(std::unique_ptr<DebuggerBase>(debugger));
        m_debugger_enabled_states.emplace_back(debugger->IsEnabled());
    }

    void DebuggerRegistry::RemoveDebugger(std::string_view debugger_name)
    {
        for (size_t i = 0; i < m_debuggers.size(); i++)
        {
            if (m_debuggers[i]->Name() == debugger_name)
            {
                m_debuggers.erase(m_debuggers.begin() + i);
                m_debugger_enabled_states.erase(m_debugger_enabled_states.begin() + i);
                return;
            }
        }
    }

    void DebuggerRegistry::SetDebuggingEnabled(bool enabled)
    {
        if (m_enabled == enabled)
        {
            return;
        }
        m_enabled = enabled;

        if (enabled)
        {
            for (const std::unique_ptr<DebuggerBase>& debugger : m_debuggers)
            {
                if (debugger->IsEnabled())
                {
                    debugger->OnEnabled();
                }
            }
        }
        else
        {
            for (const std::unique_ptr<DebuggerBase>& debugger : m_debuggers)
            {
                if (debugger->IsEnabled())
                {
                    debugger->OnDisabled();
                }
            }
        }
    }

    void DebuggerRegistry::ImGui()
    {
        if (m_enabled == false)
        {
            return;
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Debuggers"))
            {
                // TODO: categorise
                for (const std::unique_ptr<DebuggerBase>& debugger : m_debuggers)
                {
                    bool enabled = debugger->IsEnabled();
                    if (ImGui::Checkbox(debugger->Name().c_str(), &enabled))
                    {
                        SetDebuggerEnabled(*debugger, enabled);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        for (const std::unique_ptr<DebuggerBase>& debugger : m_debuggers)
        {
            if (debugger->IsEnabled())
            {
                bool enabled = true;
                debugger->ImGui(enabled);
                SetDebuggerEnabled(*debugger, enabled);
            }
        }
    }

    void DebuggerRegistry::Draw(const double delta_time_seconds)
    {
        if (m_enabled == false)
        {
            return;
        }

        for (const std::unique_ptr<DebuggerBase>& debugger : m_debuggers)
        {
            if (debugger->IsEnabled())
            {
                debugger->Draw(delta_time_seconds);
            }
        }
    }

    void DebuggerRegistry::SetDebuggerEnabled(DebuggerBase& debugger, const bool enabled)
    {
        if (debugger.IsEnabled() == enabled)
        {
            return;
        }

        debugger.SetEnabled(enabled);
        if (enabled)
        {
            debugger.OnEnabled();
        }
        else
        {
            debugger.OnDisabled();
        }
    }
} // namespace Debug