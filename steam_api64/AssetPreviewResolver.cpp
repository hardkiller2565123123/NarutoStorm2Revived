#include "StdInc.h"
#include "AssetPreviewResolver.h"
#include "AssetBrowser.h"
#include "Logger.h"

#include <filesystem>

namespace
{
    bool Exists(const std::string& path)
    {
        std::error_code ec;
        return !path.empty() && std::filesystem::exists(path, ec);
    }

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    std::filesystem::path BuildExpectedDumpPath(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path out = AssetBrowser::GetDumpFolder();

        if (asset.IsCpkEntry)
        {
            out /= "CPK";
            out /= std::filesystem::path(asset.ArchivePath).stem();
            out /= Normalize(asset.ArchiveEntryPath);
            return out;
        }

        if (!asset.VirtualPath.empty())
            out /= Normalize(asset.VirtualPath);
        else if (!asset.RelativePath.empty())
            out /= Normalize(asset.RelativePath);
        else
            out /= asset.Name;

        return out;
    }
}

namespace AssetPreviewResolver
{
    bool ResolveReadablePath(
        const AssetBrowser::AssetEntry& asset,
        std::string& outPath,
        bool autoDumpIfNeeded)
    {
        outPath.clear();

        if (Exists(asset.OverrideFullPath))
        {
            outPath = asset.OverrideFullPath;
            return true;
        }

        if (Exists(asset.DumpFullPath))
        {
            outPath = asset.DumpFullPath;
            return true;
        }

        if (!asset.IsCpkEntry && Exists(asset.FullPath))
        {
            outPath = asset.FullPath;
            return true;
        }

        std::filesystem::path expectedDump = BuildExpectedDumpPath(asset);

        if (Exists(expectedDump.string()))
        {
            outPath = expectedDump.string();
            return true;
        }

        if (!autoDumpIfNeeded)
            return false;

        Logger::Info("AssetPreviewResolver auto-dumping asset for preview: " + asset.VirtualPath);

        if (!AssetBrowser::DumpAsset(asset, true))
            return false;

        if (Exists(expectedDump.string()))
        {
            outPath = expectedDump.string();
            return true;
        }

        return false;
    }
}