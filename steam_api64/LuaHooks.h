#pragma once
#include "StdInc.h"

namespace LuaHooks
{
    bool Init();
    void Shutdown();

    void LogScriptRequest(const std::string& virtualPath);
    bool ResolveOverride(const std::string& virtualPath, std::string& outOverridePath);

    const std::vector<std::string>& GetLoadLog();

    bool Enabled();
    void SetEnabled(bool enabled);

    bool RedirectEnabled();
    void SetRedirectEnabled(bool enabled);
}