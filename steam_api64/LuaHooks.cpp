#include "StdInc.h"
#include "LuaHooks.h"

#include "Logger.h"
#include "LuaManager.h"

#include <filesystem>

namespace
{
    bool g_Enabled = true;
    bool g_RedirectEnabled = true;
    std::vector<std::string> g_LoadLog;

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }
}

namespace LuaHooks
{
    bool Init()
    {
        g_LoadLog.clear();
        Logger::Info("LuaHooks initialized");
        return true;
    }

    void Shutdown()
    {
        g_LoadLog.clear();
        Logger::Info("LuaHooks shutdown");
    }

    void LogScriptRequest(const std::string& virtualPath)
    {
        if (!g_Enabled)
            return;

        std::string path = Normalize(virtualPath);
        g_LoadLog.push_back(path);

        if (g_LoadLog.size() > 300)
            g_LoadLog.erase(g_LoadLog.begin(), g_LoadLog.begin() + 50);

        Logger::Info("LuaHooks script requested: " + path);
    }

    bool ResolveOverride(const std::string& virtualPath, std::string& outOverridePath)
    {
        outOverridePath.clear();

        if (!g_Enabled || !g_RedirectEnabled)
            return false;

        std::string normalized = Normalize(virtualPath);

        std::filesystem::path candidate =
            std::filesystem::path(LuaManager::GetOverrideFolder()) / normalized;

        if (!std::filesystem::exists(candidate))
            return false;

        outOverridePath = candidate.string();
        Logger::Info("LuaHooks override resolved: " + normalized + " -> " + outOverridePath);
        return true;
    }

    const std::vector<std::string>& GetLoadLog()
    {
        return g_LoadLog;
    }

    bool Enabled()
    {
        return g_Enabled;
    }

    void SetEnabled(bool enabled)
    {
        g_Enabled = enabled;
    }

    bool RedirectEnabled()
    {
        return g_RedirectEnabled;
    }

    void SetRedirectEnabled(bool enabled)
    {
        g_RedirectEnabled = enabled;
    }
}