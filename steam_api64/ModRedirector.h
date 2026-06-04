#pragma once
#include "StdInc.h"

namespace ModRedirector
{
    struct RedirectEntry
    {
        std::string VirtualPath;
        std::string ReplacementPath;
        std::string SourceName;
        bool IsPackage = false;
    };

    bool Init();
    void Shutdown();
    void Scan();
    void DumpToLog();

    bool IsEnabled();
    void SetEnabled(bool enabled);

    bool ResolvePath(const std::string& requestedVirtualPath, std::string& outReplacementPath);
    const std::vector<RedirectEntry>& GetRedirects();
    const std::string& GetModsFolder();
}
