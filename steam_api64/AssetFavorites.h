#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetFavorites
{
    bool Init();
    void Shutdown();
    bool Load();
    bool Save();

    std::string KeyFor(const AssetBrowser::AssetEntry& asset);

    bool IsFavorite(const AssetBrowser::AssetEntry& asset);
    void ToggleFavorite(const AssetBrowser::AssetEntry& asset);
    void SetFavorite(const AssetBrowser::AssetEntry& asset, bool favorite);

    void AddRecent(const AssetBrowser::AssetEntry& asset);

    std::vector<std::string> GetFavorites();
    std::vector<std::string> GetRecent();
}
