#include "StdInc.h"
#include "AssetDuplicateFinder.h"
#include "AssetBrowser.h"
#include "AssetHash.h"
#include "AssetNotification.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace
{
    std::vector<AssetDuplicateFinder::DuplicateGroup> g_Groups;

    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return s;
    }

    void BuildFromMap(const std::unordered_map<std::string, std::vector<int>>& map)
    {
        g_Groups.clear();
        const auto& assets = AssetBrowser::GetAssets();

        for (const auto& pair : map)
        {
            if (pair.second.size() < 2)
                continue;

            AssetDuplicateFinder::DuplicateGroup g{};
            g.Key = pair.first;
            g.AssetIndexes = pair.second;

            for (int index : pair.second)
                if (index >= 0 && index < static_cast<int>(assets.size()))
                    g.TotalSize += assets[index].Size;

            g_Groups.push_back(g);
        }

        std::sort(g_Groups.begin(), g_Groups.end(), [](const auto& a, const auto& b){ return a.TotalSize > b.TotalSize; });
    }
}

namespace AssetDuplicateFinder
{
    bool Init(){ return true; }
    void Shutdown(){ g_Groups.clear(); }

    void RebuildByName()
    {
        std::unordered_map<std::string, std::vector<int>> map;
        const auto& assets = AssetBrowser::GetAssets();
        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
            map[Lower(assets[i].Name)].push_back(i);
        BuildFromMap(map);
        AssetNotification::PushInfo("Duplicate Finder", "Groups by name: " + std::to_string(g_Groups.size()));
    }

    void RebuildBySize()
    {
        std::unordered_map<std::string, std::vector<int>> map;
        const auto& assets = AssetBrowser::GetAssets();
        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
            if (assets[i].Size > 0) map[std::to_string(assets[i].Size)].push_back(i);
        BuildFromMap(map);
        AssetNotification::PushInfo("Duplicate Finder", "Groups by size: " + std::to_string(g_Groups.size()));
    }

    void RebuildByHash()
    {
        std::unordered_map<std::string, std::vector<int>> map;
        const auto& assets = AssetBrowser::GetAssets();

        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
        {
            AssetHash::HashInfo h{};
            if (AssetHash::ComputeAsset(assets[i], h))
                map[h.FNV1A64].push_back(i);
        }

        BuildFromMap(map);
        AssetNotification::PushInfo("Duplicate Finder", "Groups by hash: " + std::to_string(g_Groups.size()));
    }

    const std::vector<DuplicateGroup>& GetGroups(){ return g_Groups; }

    bool ExportReport(const std::string& outputPath)
    {
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
        std::ofstream f(outputPath, std::ios::trunc);
        if (!f.is_open()) return false;

        const auto& assets = AssetBrowser::GetAssets();
        for (const auto& g : g_Groups)
        {
            f << "GROUP: " << g.Key << " Count=" << g.AssetIndexes.size() << " TotalSize=" << g.TotalSize << "\\n";
            for (int index : g.AssetIndexes)
                if (index >= 0 && index < static_cast<int>(assets.size()))
                    f << "  - " << assets[index].VirtualPath << "\\n";
            f << "\\n";
        }
        return true;
    }
}
