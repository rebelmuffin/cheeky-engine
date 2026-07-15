#include "Utilities/Log.h"

#include <iostream>
#include <ostream>
#include <sstream>

namespace
{
    void Stdout(const std ::string& message) { std::cout << message << std::endl; }
    void Stderr(const std::string& message) { std::cerr << message << std::endl; }
} // namespace

namespace Utilities
{
    void LogError(std::string_view message)
    {
        std::stringstream ss;
        ss << "[!] " << message;
        Stderr(ss.str());
    }

    void LogWarning(std::string_view message)
    {
        std::stringstream ss;
        ss << "[W] " << message;
        Stdout(ss.str());
    }

    void LogInfo(std::string_view message)
    {
        std::stringstream ss;
        ss << "[I] " << message;
        Stdout(ss.str());
    }
} // namespace Utilities