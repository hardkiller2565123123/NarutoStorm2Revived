#pragma once
#include "StdInc.h"
#include "CpkArchive.h"

namespace AssetBrowser
{
    enum class AssetType
    {
        Unknown,
        Package,
        Model,
        Texture,
        Animation,
        Audio,
        Stage,
        Character,
        Data,
        CpkEntry
    };

    enum class AssetSource
    {
        Game = 0,
        Mod = 1,
        Dump = 2,
        PackageInner = 3
    };

    struct AssetEntry
    {
        std::string Name;
        std::string FullPath;
        std::string RelativePath;
        std::string VirtualPath;
        std::string Extension;
        AssetType Type = AssetType::Unknown;
        AssetSource Source = AssetSource::Game;
        uintmax_t Size = 0;
        bool HasModOverride = false;
        bool IsLooseOverride = false;
        bool IsDumped = false;
        bool IsVirtualPackageEntry = false;
        bool CanDump = true;
        std::string OverrideFullPath;
        std::string DumpFullPath;
        CpkArchive::CpkEntry CpkEntryData;
    };

    struct DumpEntry
    {
        std::string Name;
        std::string FullPath;
        std::string RelativePath;
        std::string Extension;
        AssetType Type = AssetType::Unknown;
        uintmax_t Size = 0;
    };

    struct AssetStats
    {
        int TotalAssets = 0;
        int GameAssets = 0;
        int ModAssets = 0;
        int DumpedAssets = 0;
        int OverrideAssets = 0;
        int PackageAssets = 0;
        int PackageInnerAssets = 0;
        int XfbinAssets = 0;
        int TextureAssets = 0;
        int AudioAssets = 0;
        int StageAssets = 0;
        int CharacterAssets = 0;
        int ModelAssets = 0;
    };

    bool Init();
    void Scan();
    void ScanDumpFolder();
    void DumpToLog();
    bool ExportCsv(const std::string& relativeOrFullPath);

    bool DumpAsset(const AssetEntry& asset, bool tryExtractArchives = true);
    bool DumpAssetByIndex(int index, bool tryExtractArchives = true);
    int DumpAllAssets(bool tryExtractArchives = false);
    int DumpAssetsByType(AssetType type, bool tryExtractArchives = false);
    bool DumpXfbinInfo(const AssetEntry& asset);
    bool ExtractCpkIfToolExists(const AssetEntry& asset); // legacy name; now built-in where possible
    bool LoadCpkEntries(const AssetEntry& packageAsset);

    std::string BuildAssetInfoText(const AssetEntry& asset);
    std::string ReadSmallTextFile(const std::string& path, size_t maxBytes = 32768);
    bool IsTextPreviewable(const AssetEntry& asset);
    bool IsTexturePreviewable(const AssetEntry& asset);
    bool IsModelPreviewable(const AssetEntry& asset);

    const std::vector<AssetEntry>& GetAssets();
    const std::vector<DumpEntry>& GetDumpedFiles();
    const std::vector<CpkArchive::CpkInfo>& GetLoadedPackages();
    AssetStats GetStats();

    const std::string& GetGameFolder();
    const std::string& GetModsFolder();
    const std::string& GetDumpFolder();
    const std::string& GetToolsFolder();

    const char* TypeName(AssetType type);
    const char* SourceName(AssetSource source);
}
