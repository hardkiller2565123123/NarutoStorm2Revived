#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetOverrideManager
{
    struct OverrideEntry
    {
        std::string VirtualPath;
        std::string OriginalPath;
        std::string OverridePath;
        bool Enabled = true;
        bool Exists = false;
    };

    bool Init();
    void Shutdown();

    std::filesystem::path GetOverrideRoot();
    std::filesystem::path BuildOverridePath(const AssetBrowser::AssetEntry& asset);
    std::filesystem::path BuildDisabledPath(const AssetBrowser::AssetEntry& asset);

    bool CreateOverrideFromDump(const AssetBrowser::AssetEntry& asset);
    bool CreateOverrideFromFile(const AssetBrowser::AssetEntry& asset, const std::string& sourceFile);
    bool DisableOverride(const AssetBrowser::AssetEntry& asset);
    bool EnableOverride(const AssetBrowser::AssetEntry& asset);
    bool DeleteOverride(const AssetBrowser::AssetEntry& asset);

    bool HasOverride(const AssetBrowser::AssetEntry& asset);
    std::string GetOverridePath(const AssetBrowser::AssetEntry& asset);

    std::vector<OverrideEntry> BuildOverrideList();
}
