#pragma once
#include "StdInc.h"

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
        Data
    };

    enum class AssetSource
    {
        Game = 0,
        Mod = 1,
        Dump = 2
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
        std::string OverrideFullPath;
        std::string DumpFullPath;
        bool IsCpkEntry = false;
        std::string ArchivePath;
        std::string ArchiveEntryPath;
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

    bool LoadCache();
    bool SaveCache();
    void StartAsyncScan(bool forceFullRefresh = false);
    bool IsScanning();
    int GetScanProgress();
    std::string GetScanStatus();
    bool WasCacheLoaded();
    void StartAsyncPreload();
    bool IsPreloading();
    int GetPreloadProgress();
    std::string GetPreloadStatus();

    void DumpToLog();
    bool ExportCsv(const std::string& relativeOrFullPath);

    bool DumpAsset(const AssetEntry& asset, bool tryExtractArchives = true);
    bool DumpAssetByIndex(int index, bool tryExtractArchives = true);
    int DumpAllAssets(bool tryExtractArchives = false);
    int DumpAssetsByType(AssetType type, bool tryExtractArchives = false);
    bool DumpXfbinInfo(const AssetEntry& asset);
    bool ExtractCpkIfToolExists(const AssetEntry& asset);

    std::string BuildAssetInfoText(const AssetEntry& asset);
    std::string ReadSmallTextFile(const std::string& path, size_t maxBytes = 32768);
    bool IsTextPreviewable(const AssetEntry& asset);
    bool IsTexturePreviewable(const AssetEntry& asset);
    bool IsModelPreviewable(const AssetEntry& asset);

    const std::vector<AssetEntry>& GetAssets();
    const std::vector<DumpEntry>& GetDumpedFiles();
    AssetStats GetStats();

    const std::string& GetGameFolder();
    const std::string& GetModsFolder();
    const std::string& GetDumpFolder();
    const std::string& GetToolsFolder();

    const char* TypeName(AssetType type);
    const char* SourceName(AssetSource source);
}
