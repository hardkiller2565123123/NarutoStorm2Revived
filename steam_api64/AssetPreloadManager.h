#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"
#include "AssetPreview.h"

namespace AssetPreloadManager
{
    struct PreloadSettings
    {
        bool Enabled = true;
        bool FullEagerPreload = true;
        bool BlockingStartupLoad = false;
        bool KeepPreviewCacheInMemory = true;

        bool PreloadModels = true;
        bool PreloadTextures = true;
        bool PreloadDataAndHex = true;
        bool PreloadPackages = true;
        bool PreloadAudio = true;

        size_t MaxBytesKeptPerAsset = 1024 * 1024;
    };

    bool Init();
    void Shutdown();

    void StartFullPreload(bool blocking = false);
    void StopPreload();

    bool IsPreloading();
    bool IsComplete();

    int GetProgress();
    int GetLoadedCount();
    int GetTotalCount();

    std::string GetStatus();
    std::string GetCurrentAsset();

    const PreloadSettings& GetSettings();
    PreloadSettings& MutableSettings();

    bool GetCachedPreview(const AssetBrowser::AssetEntry& asset, AssetPreview::PreviewState& outState);
    bool ForceBuildPreview(const AssetBrowser::AssetEntry& asset, AssetPreview::PreviewState& outState);

    void ClearCache();
}
