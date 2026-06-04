#include "StdInc.h"
#include "SteamVersionLogger.h"
#include "Logger.h"

namespace SteamVersionLogger
{
    void LogExportRequest(const char* name)
    {
        Logger::Info(std::string("[STEAM EXPORT] ") + (name ? name : "null"));
    }

    void LogInterfaceRequest(const char* source, const char* version)
    {
        Logger::Info(
            std::string("[STEAM INTERFACE] ") +
            (source ? source : "unknown") +
            " requested: " +
            (version ? version : "null"));
    }

    bool Contains(const std::string& text, const char* needle)
    {
        if (!needle)
            return false;

        return text.find(needle) != std::string::npos;
    }
}