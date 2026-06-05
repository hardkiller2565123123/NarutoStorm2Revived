#pragma once
#include "StdInc.h"

namespace ModRedirector
{
    struct OverrideEntry
    {
        std::string VirtualPath;
        std::string FullPath;
        std::string SourceName;
        bool Enabled = true;
    };

    bool Init();
    void Scan();
    void Shutdown();

    bool Resolve(const std::string& virtualPath, std::string& outFullPath);
    bool HasOverride(const std::string& virtualPath);
    const std::vector<OverrideEntry>& GetOverrides();
    const std::string& GetModsFolder();
}
