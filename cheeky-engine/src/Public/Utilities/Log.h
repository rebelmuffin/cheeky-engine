#pragma once

#include <format>

namespace Utilities
{
    void LogError(std::string_view message);
    void LogWarning(std::string_view message);
    void LogInfo(std::string_view message);

    template <typename... Args>
    void LogErrorFmt(std::format_string<Args...> fmt, Args&&... args)
    {
        LogError(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void LogWarningFmt(std::format_string<Args...> fmt, Args&&... args)
    {
        LogWarning(std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void LogInfoFmt(std::format_string<Args...> fmt, Args&&... args)
    {
        LogInfo(std::format(fmt, std::forward<Args>(args)...));
    }
} // namespace Utilities

// log macroes. Can be completely stripped out of compilation
#ifdef DISABLE_LOGGING
#define LOG_ERROR(__VA_ARGS__)
#define LOG_WARNING(__VA_ARGS__)
#define LOG_INFO(__VA_ARGS__)
#else
#define LOG_ERROR(...) ::Utilities::LogErrorFmt(__VA_ARGS__)
#define LOG_WARNING(...) ::Utilities::LogWarningFmt(__VA_ARGS__)
#define LOG_INFO(...) ::Utilities::LogInfoFmt(__VA_ARGS__)
#endif