#include "Game/GameLogging.h"

namespace
{
    Game::LogSeverity& SeverityContainer()
    {
        static Game::LogSeverity severity = Game::LogSeverity::Error;
        return severity;
    }
} // namespace

namespace Game
{
    LogSeverity LoggingSeverity() { return SeverityContainer(); }
    void SetLoggingSeverity(LogSeverity new_severity) { SeverityContainer() = new_severity; }

    bool SeverityEnabled(LogSeverity severity)
    {
        uint8_t current_severity = (uint8_t)LogSeverity();
        uint8_t needed_severity = (uint8_t)severity;
        return current_severity >= needed_severity;
    }

    bool InfoLogEnabled() { return SeverityEnabled(LogSeverity::Info); }
    bool WarningsEnabled() { return SeverityEnabled(LogSeverity::Warning); }
    bool ErrorsEnabled() { return SeverityEnabled(LogSeverity::Error); }
} // namespace Game