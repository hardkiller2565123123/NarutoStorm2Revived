#include "StdInc.h"
#include "AssetPreloadManager.h"
#include "Logger.h"

#include <thread>
#include <atomic>
#include <mutex>

namespace
{
    AssetPreloadManager::PreloadSettings g_Settings;

    std::thread g_Thread;
    std::mutex g_Mutex;

    std::atomic<bool> g_Running(false);
    std::atomic<bool> g_StopRequested(false);
    std::atomic<bool> g_Complete(false);
    std::atomic<int> g_Progress(0);
    std::atomic<int> g_LoadedCount(0);
    std::atomic<int> g_TotalCount(0);

    std::string g_Status = "Idle";
    std::string g_CurrentAsset;

    std::unordered_map<std::string, AssetPreview::PreviewState> g_Cache;

    std::string Normalize(std::string value)
    {
        std::replace(value.begin(), value.end(), '\\', '/');
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    std::string MakeKey(const AssetBrowser::AssetEntry& asset)
    {
        if (!asset.VirtualPath.empty())
            return Normalize(asset.VirtualPath);

        if (!asset.ArchiveEntryPath.empty())
            return Normalize(asset.ArchivePath + ":" + asset.ArchiveEntryPath);

        return Normalize(asset.FullPath);
    }

    bool ShouldPreload(const AssetBrowser::AssetEntry& asset)
    {
        using namespace AssetBrowser;

        if (asset.Type == AssetType::Model || asset.Type == AssetType::Character || asset.Type == AssetType::Stage)
            return g_Settings.PreloadModels;

        if (asset.Type == AssetType::Texture)
            return g_Settings.PreloadTextures;

        if (asset.Type == AssetType::Package)
            return g_Settings.PreloadPackages;

        if (asset.Type == AssetType::Audio)
            return g_Settings.PreloadAudio;

        if (asset.Type == AssetType::Data || asset.Type == AssetType::Animation || asset.Type == AssetType::Unknown)
            return g_Settings.PreloadDataAndHex;

        return true;
    }

    void TrimPreviewForMemory(AssetPreview::PreviewState& state)
    {
        if (g_Settings.MaxBytesKeptPerAsset > 0 && state.Bytes.size() > g_Settings.MaxBytesKeptPerAsset)
        {
            state.Bytes.resize(g_Settings.MaxBytesKeptPerAsset);
            state.Status += " | bytes trimmed for memory cache";
        }

        const size_t maxText = 1024 * 1024;
        if (state.Text.size() > maxText)
        {
            state.Text.resize(maxText);
            state.Text += "\n\n[trimmed for memory cache]\n";
        }
    }

    void SetStatus(const std::string& status, const std::string& current = std::string())
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Status = status;
        g_CurrentAsset = current;
    }

    void PreloadWorker()
    {
        g_Running = true;
        g_Complete = false;
        g_StopRequested = false;
        g_Progress = 0;
        g_LoadedCount = 0;

        std::vector<AssetBrowser::AssetEntry> assets = AssetBrowser::GetAssets();

        std::vector<AssetBrowser::AssetEntry> work;
        work.reserve(assets.size());

        for (const auto& asset : assets)
        {
            if (ShouldPreload(asset))
                work.push_back(asset);
        }

        g_TotalCount = static_cast<int>(work.size());
        Logger::Info("AssetPreloadManager full preload started. Assets=" + std::to_string(g_TotalCount.load()));

        if (work.empty())
        {
            SetStatus("No assets to preload");
            g_Progress = 100;
            g_Complete = true;
            g_Running = false;
            return;
        }

        for (int i = 0; i < static_cast<int>(work.size()); i++)
        {
            if (g_StopRequested)
                break;

            const auto& asset = work[i];
            std::string display = !asset.VirtualPath.empty() ? asset.VirtualPath : asset.Name;
            SetStatus("Preloading asset previews", display);

            AssetPreview::PreviewState preview{};
            if (AssetPreview::BuildPreview(asset, preview, true))
            {
                TrimPreviewForMemory(preview);

                if (g_Settings.KeepPreviewCacheInMemory)
                {
                    std::lock_guard<std::mutex> lock(g_Mutex);
                    g_Cache[MakeKey(asset)] = preview;
                }
            }

            g_LoadedCount = i + 1;
            g_Progress = static_cast<int>((static_cast<double>(i + 1) / static_cast<double>(work.size())) * 100.0);
        }

        if (g_StopRequested)
        {
            SetStatus("Asset preload stopped");
            Logger::Info("AssetPreloadManager stopped");
        }
        else
        {
            g_Progress = 100;
            SetStatus("Asset preload complete");
            Logger::Info("AssetPreloadManager complete. Cached=" + std::to_string(g_Cache.size()));
        }

        g_Complete = !g_StopRequested;
        g_Running = false;
    }
}

namespace AssetPreloadManager
{
    bool Init()
    {
        Logger::Info("AssetPreloadManager initialized");
        return true;
    }

    void Shutdown()
    {
        StopPreload();
        ClearCache();
        Logger::Info("AssetPreloadManager shutdown");
    }

    void StartFullPreload(bool blocking)
    {
        if (!g_Settings.Enabled)
            return;

        if (g_Running)
            return;

        StopPreload();

        if (blocking || g_Settings.BlockingStartupLoad)
        {
            PreloadWorker();
            return;
        }

        g_Thread = std::thread(PreloadWorker);
    }

    void StopPreload()
    {
        g_StopRequested = true;

        if (g_Thread.joinable())
            g_Thread.join();

        g_Running = false;
    }

    bool IsPreloading() { return g_Running; }
    bool IsComplete() { return g_Complete; }
    int GetProgress() { return g_Progress; }
    int GetLoadedCount() { return g_LoadedCount; }
    int GetTotalCount() { return g_TotalCount; }

    std::string GetStatus()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_Status;
    }

    std::string GetCurrentAsset()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_CurrentAsset;
    }

    const PreloadSettings& GetSettings() { return g_Settings; }
    PreloadSettings& MutableSettings() { return g_Settings; }

    bool GetCachedPreview(const AssetBrowser::AssetEntry& asset, AssetPreview::PreviewState& outState)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        auto it = g_Cache.find(MakeKey(asset));
        if (it == g_Cache.end())
            return false;
        outState = it->second;
        return true;
    }

    bool ForceBuildPreview(const AssetBrowser::AssetEntry& asset, AssetPreview::PreviewState& outState)
    {
        if (GetCachedPreview(asset, outState))
            return true;

        if (!AssetPreview::BuildPreview(asset, outState, true))
            return false;

        TrimPreviewForMemory(outState);

        if (g_Settings.KeepPreviewCacheInMemory)
        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            g_Cache[MakeKey(asset)] = outState;
        }

        return true;
    }

    void ClearCache()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Cache.clear();
    }
}
