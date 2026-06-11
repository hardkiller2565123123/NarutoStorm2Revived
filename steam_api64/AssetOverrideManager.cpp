#include "StdInc.h"
#include "AssetOverrideManager.h"
#include "AssetPreviewResolver.h"
#include "AssetNotification.h"
#include "Logger.h"

#include <filesystem>

namespace
{
    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');

        while (!s.empty() && (s[0] == '/' || s[0] == '.'))
            s.erase(s.begin());

        return s;
    }

    bool Exists(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    bool CopySafe(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        std::error_code ec;
        std::filesystem::create_directories(to.parent_path(), ec);
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
        return !ec;
    }
}

namespace AssetOverrideManager
{
    bool Init()
    {
        std::filesystem::create_directories(GetOverrideRoot());
        Logger::Info("AssetOverrideManager initialized: " + GetOverrideRoot().string());
        return true;
    }

    void Shutdown()
    {
        Logger::Info("AssetOverrideManager shutdown");
    }

    std::filesystem::path GetOverrideRoot()
    {
        return std::filesystem::path(AssetBrowser::GetModsFolder()) / "assets";
    }

    std::filesystem::path BuildOverridePath(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path root = GetOverrideRoot();

        if (!asset.ArchiveEntryPath.empty())
            return root / Normalize(asset.ArchiveEntryPath);

        if (!asset.VirtualPath.empty())
            return root / Normalize(asset.VirtualPath);

        if (!asset.RelativePath.empty())
            return root / Normalize(asset.RelativePath);

        return root / asset.Name;
    }

    std::filesystem::path BuildDisabledPath(const AssetBrowser::AssetEntry& asset)
    {
        return BuildOverridePath(asset).string() + ".disabled";
    }

    bool CreateOverrideFromDump(const AssetBrowser::AssetEntry& asset)
    {
        std::string readable;

        if (!AssetPreviewResolver::ResolveReadablePath(asset, readable, true))
        {
            AssetNotification::PushError("Override", "Could not resolve/dump readable asset.");
            return false;
        }

        return CreateOverrideFromFile(asset, readable);
    }

    bool CreateOverrideFromFile(const AssetBrowser::AssetEntry& asset, const std::string& sourceFile)
    {
        std::filesystem::path out = BuildOverridePath(asset);

        if (!CopySafe(sourceFile, out))
        {
            AssetNotification::PushError("Override", "Failed to create override copy.");
            return false;
        }

        AssetNotification::PushSuccess("Override Created", out.string());
        Logger::Info("Override created: " + out.string());
        return true;
    }

    bool DisableOverride(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path p = BuildOverridePath(asset);
        std::filesystem::path d = BuildDisabledPath(asset);

        if (!Exists(p))
            return false;

        std::error_code ec;
        std::filesystem::rename(p, d, ec);
        return !ec;
    }

    bool EnableOverride(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path p = BuildOverridePath(asset);
        std::filesystem::path d = BuildDisabledPath(asset);

        if (!Exists(d))
            return false;

        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        std::filesystem::rename(d, p, ec);
        return !ec;
    }

    bool DeleteOverride(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path p = BuildOverridePath(asset);

        if (!Exists(p))
            return false;

        std::error_code ec;
        std::filesystem::remove(p, ec);
        return !ec;
    }

    bool HasOverride(const AssetBrowser::AssetEntry& asset)
    {
        return Exists(BuildOverridePath(asset));
    }

    std::string GetOverridePath(const AssetBrowser::AssetEntry& asset)
    {
        return BuildOverridePath(asset).string();
    }

    std::vector<OverrideEntry> BuildOverrideList()
    {
        std::vector<OverrideEntry> out;
        const auto& assets = AssetBrowser::GetAssets();

        for (const auto& asset : assets)
        {
            std::filesystem::path p = BuildOverridePath(asset);
            std::filesystem::path d = BuildDisabledPath(asset);

            bool enabled = Exists(p);
            bool disabled = Exists(d);

            if (!enabled && !disabled && !asset.HasModOverride && !asset.IsLooseOverride)
                continue;

            OverrideEntry e{};
            e.VirtualPath = !asset.ArchiveEntryPath.empty() ? asset.ArchiveEntryPath : asset.VirtualPath;
            e.OriginalPath = asset.FullPath;
            e.OverridePath = enabled ? p.string() : d.string();
            e.Enabled = enabled;
            e.Exists = true;
            out.push_back(e);
        }

        return out;
    }
}
