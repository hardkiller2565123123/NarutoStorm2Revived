#include "StdInc.h"
#include "AssetFavorites.h"
#include "AssetNotification.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <set>

namespace
{
    std::set<std::string> g_Favorites;
    std::vector<std::string> g_Recent;

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    std::filesystem::path ConfigRoot()
    {
        return std::filesystem::path(AssetBrowser::GetGameFolder()) / "NartuoStorm2Revived" / "Config";
    }

    std::filesystem::path FavPath()
    {
        return ConfigRoot() / "asset_favorites.txt";
    }

    std::filesystem::path RecentPath()
    {
        return ConfigRoot() / "asset_recent.txt";
    }
}

namespace AssetFavorites
{
    bool Init()
    {
        std::filesystem::create_directories(ConfigRoot());
        Load();
        Logger::Info("AssetFavorites initialized");
        return true;
    }

    void Shutdown()
    {
        Save();
        Logger::Info("AssetFavorites shutdown");
    }

    std::string KeyFor(const AssetBrowser::AssetEntry& asset)
    {
        if (!asset.ArchiveEntryPath.empty())
            return Normalize(asset.ArchivePath + ":" + asset.ArchiveEntryPath);
        if (!asset.VirtualPath.empty())
            return Normalize(asset.VirtualPath);
        return Normalize(asset.FullPath);
    }

    bool IsFavorite(const AssetBrowser::AssetEntry& asset)
    {
        return g_Favorites.count(KeyFor(asset)) > 0;
    }

    void ToggleFavorite(const AssetBrowser::AssetEntry& asset)
    {
        SetFavorite(asset, !IsFavorite(asset));
    }

    void SetFavorite(const AssetBrowser::AssetEntry& asset, bool favorite)
    {
        std::string key = KeyFor(asset);
        if (favorite)
        {
            g_Favorites.insert(key);
            AssetNotification::PushSuccess("Favorite Added", asset.Name);
        }
        else
        {
            g_Favorites.erase(key);
            AssetNotification::PushInfo("Favorite Removed", asset.Name);
        }
        Save();
    }

    void AddRecent(const AssetBrowser::AssetEntry& asset)
    {
        std::string key = KeyFor(asset);
        g_Recent.erase(std::remove(g_Recent.begin(), g_Recent.end(), key), g_Recent.end());
        g_Recent.insert(g_Recent.begin(), key);
        if (g_Recent.size() > 100)
            g_Recent.resize(100);
        Save();
    }

    std::vector<std::string> GetFavorites()
    {
        return std::vector<std::string>(g_Favorites.begin(), g_Favorites.end());
    }

    std::vector<std::string> GetRecent()
    {
        return g_Recent;
    }

    bool Save()
    {
        std::filesystem::create_directories(ConfigRoot());
        {
            std::ofstream f(FavPath(), std::ios::trunc);
            if (!f.is_open()) return false;
            for (const auto& x : g_Favorites) f << x << "\\n";
        }
        {
            std::ofstream f(RecentPath(), std::ios::trunc);
            if (!f.is_open()) return false;
            for (const auto& x : g_Recent) f << x << "\\n";
        }
        return true;
    }

    bool Load()
    {
        g_Favorites.clear();
        g_Recent.clear();

        std::string line;
        {
            std::ifstream f(FavPath());
            while (std::getline(f, line))
                if (!line.empty()) g_Favorites.insert(line);
        }
        {
            std::ifstream f(RecentPath());
            while (std::getline(f, line))
                if (!line.empty()) g_Recent.push_back(line);
        }
        return true;
    }
}
