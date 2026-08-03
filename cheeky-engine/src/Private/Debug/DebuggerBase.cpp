#include "Debug/DebuggerBase.h"

namespace Debug
{
    DebuggerBase::DebuggerBase(std::string_view name, DebuggerCategory category, bool enabled_by_default) :
        m_name(name),
        m_category(category),
        m_enabled(enabled_by_default)
    {
    }
} // namespace Debug