#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetTaskQueue
{
    enum class TaskType
    {
        DumpAsset,
        DumpMany,
        RebuildCache,
        ExportCsv,
        PreloadAssets
    };

    struct Task
    {
        TaskType Type = TaskType::DumpAsset;
        std::string Name;
        std::vector<AssetBrowser::AssetEntry> Assets;
        std::string OutputPath;
    };

    bool Init();
    void Shutdown();

    void EnqueueDumpAsset(const AssetBrowser::AssetEntry& asset);
    void EnqueueDumpMany(const std::vector<AssetBrowser::AssetEntry>& assets, const std::string& name);
    void EnqueueExportCsv(const std::string& outputPath);
    void EnqueueRebuildCache();

    bool IsBusy();
    int GetProgress();
    std::string GetStatus();
    std::string GetCurrentTaskName();
}