#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetPackageCreator
{
    bool Init();
    void Shutdown();

    std::filesystem::path GetModsRoot();

    std::filesystem::path CreatePackage(
        const std::string& modName,
        const std::string& author,
        const std::string& description,
        const std::vector<AssetBrowser::AssetEntry>& assets);
}
