#include "StdInc.h"
#include "AssetRelations.h"
#include "Logger.h"

#include <mutex>
#include <unordered_map>

namespace
{
    bool g_Built = false;
    std::mutex g_Mutex;

    std::unordered_map<std::string, std::vector<int>> g_ByName;
    std::unordered_map<std::string, std::vector<int>> g_ByVirtual;
    std::unordered_map<std::string, std::vector<int>> g_ByFolder;
    std::unordered_map<std::string, std::vector<int>> g_ByArchive;
    std::unordered_map<std::string, std::vector<int>> g_ByFamily;

    std::string Lower(std::string s)
    {
        std::transform(
            s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    std::string FolderOf(const std::string& path)
    {
        std::string p = Lower(path);
        size_t slash = p.find_last_of('/');

        if (slash == std::string::npos)
            return "";

        return p.substr(0, slash);
    }

    std::string NameOnly(const std::string& path)
    {
        std::string p = Lower(path);
        size_t slash = p.find_last_of('/');

        if (slash == std::string::npos)
            return p;

        return p.substr(slash + 1);
    }

    bool ContainsPathToken(const std::string& hay, const std::string& needle)
    {
        if (needle.empty())
            return false;

        std::string h = Lower(hay);
        std::string n = Lower(needle);

        if (h.find(n) != std::string::npos)
            return true;

        std::string name = NameOnly(n);

        return !name.empty() && h.find(name) != std::string::npos;
    }

    void AddUnique(std::vector<int>& list, int index)
    {
        if (index < 0)
            return;

        if (std::find(list.begin(), list.end(), index) == list.end())
            list.push_back(index);
    }
}

namespace AssetRelations
{
    bool Init()
    {
        Rebuild();
        Logger::Info("AssetRelations initialized");
        return true;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        g_ByName.clear();
        g_ByVirtual.clear();
        g_ByFolder.clear();
        g_ByArchive.clear();
        g_ByFamily.clear();
        g_Built = false;

        Logger::Info("AssetRelations shutdown");
    }

    std::string GuessFamilyKey(const AssetBrowser::AssetEntry& asset)
    {
        std::string p = Lower(asset.VirtualPath);

        if (p.empty())
            p = Lower(asset.ArchiveEntryPath);

        if (p.empty())
            p = Lower(asset.Name);

        std::vector<std::string> parts;
        std::stringstream ss(p);
        std::string item;

        while (std::getline(ss, item, '/'))
        {
            if (!item.empty())
                parts.push_back(item);
        }

        for (const auto& part : parts)
        {
            if (part.find("player") != std::string::npos ||
                part.find("char") != std::string::npos ||
                part.find("stage") != std::string::npos ||
                part.find("stg") != std::string::npos)
            {
                return part;
            }
        }

        if (parts.size() >= 2)
            return parts[parts.size() - 2];

        return NameOnly(p);
    }

    void Rebuild()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        g_ByName.clear();
        g_ByVirtual.clear();
        g_ByFolder.clear();
        g_ByArchive.clear();
        g_ByFamily.clear();

        const auto& assets = AssetBrowser::GetAssets();

        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
        {
            const auto& asset = assets[i];

            g_ByName[Lower(asset.Name)].push_back(i);
            g_ByVirtual[Lower(asset.VirtualPath)].push_back(i);
            g_ByFolder[FolderOf(asset.VirtualPath)].push_back(i);

            if (!asset.ArchivePath.empty())
                g_ByArchive[Lower(asset.ArchivePath)].push_back(i);

            g_ByFamily[GuessFamilyKey(asset)].push_back(i);
        }

        g_Built = true;

        Logger::Info("AssetRelations rebuilt. Assets=" + std::to_string(assets.size()));
    }

    bool IsBuilt()
    {
        return g_Built;
    }

    RelatedResult FindRelated(const AssetBrowser::AssetEntry& asset)
    {
        if (!g_Built)
            Rebuild();

        RelatedResult result{};
        const auto& assets = AssetBrowser::GetAssets();

        std::string assetVirtual = Lower(asset.VirtualPath);
        std::string assetArchive = Lower(asset.ArchivePath);
        std::string assetFolder = FolderOf(asset.VirtualPath);
        std::string assetFamily = GuessFamilyKey(asset);

        for (const auto& dep : asset.Dependencies)
        {
            std::string depLower = Lower(dep);
            std::string depName = NameOnly(depLower);

            for (int i = 0; i < static_cast<int>(assets.size()); ++i)
            {
                const auto& other = assets[i];

                if (ContainsPathToken(other.VirtualPath, depLower) ||
                    ContainsPathToken(other.ArchiveEntryPath, depLower) ||
                    Lower(other.Name) == depName)
                {
                    AddUnique(result.Dependencies, i);
                }
            }
        }

        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
        {
            const auto& other = assets[i];

            if (&other == &asset)
                continue;

            for (const auto& dep : other.Dependencies)
            {
                if (ContainsPathToken(dep, assetVirtual) ||
                    ContainsPathToken(dep, asset.Name))
                {
                    AddUnique(result.UsedBy, i);
                }
            }

            if (!assetArchive.empty() && Lower(other.ArchivePath) == assetArchive)
                AddUnique(result.SameArchive, i);

            if (!assetFolder.empty() && FolderOf(other.VirtualPath) == assetFolder)
                AddUnique(result.SameFolder, i);

            if (other.Type == asset.Type)
                AddUnique(result.SameType, i);

            if (!assetFamily.empty() && GuessFamilyKey(other) == assetFamily)
                AddUnique(result.SameFamily, i);
        }

        return result;
    }
}