#pragma once

#include <cstdarg>
#include <cstdint>
#include <iostream>

namespace Game
{
    // Higher severity number means lower impact logs.
    enum class LogSeverity : uint8_t
    {
        None = 0,
        Error = 1 << 0,
        Warning = 1 << 1,
        Info = 1 << 2
    };

    LogSeverity LoggingSeverity();
    void SetLoggingSeverity(LogSeverity new_severity);

    bool SeverityEnabled(LogSeverity severity);
    bool InfoLogEnabled();
    bool WarningsEnabled();
    bool ErrorsEnabled();

    inline void Log(const char* header, LogSeverity severity, const char* fmt, ...)
    {
        uint8_t current_severity = (uint8_t)LogSeverity();
        uint8_t needed_severity = (uint8_t)severity;
        if (current_severity < needed_severity)
        {
            return;
        }

        static char buffer[2048];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buffer, 2048, fmt, args);
        std::cout << header << buffer << std::endl;
        va_end(args);
    }

    inline void LogError(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        Log("[!] Game Error - ", LogSeverity::Error, fmt, args);
        va_end(args);
    }

    inline void LogWarning(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        Log("[~] Game Warning - ", LogSeverity::Warning, fmt, args);
        va_end(args);
    }

    inline void LogInfo(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        Log("[*] Game Info - ", LogSeverity::Info, fmt, args);
        va_end(args);
    }
} // namespace Game