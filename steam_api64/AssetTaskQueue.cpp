#include "StdInc.h"
#include "AssetTaskQueue.h"

#include "Logger.h"
#include "AssetPreloadManager.h"

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

namespace
{
    std::mutex g_Mutex;
    std::queue<AssetTaskQueue::Task> g_Tasks;
    std::thread g_Worker;

    std::atomic<bool> g_Running(false);
    std::atomic<bool> g_Stop(false);
    std::atomic<bool> g_Busy(false);
    std::atomic<int> g_Progress(0);

    std::string g_Status = "Idle";
    std::string g_CurrentTaskName;

    void SetStatus(const std::string& status, const std::string& taskName = std::string())
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Status = status;
        g_CurrentTaskName = taskName;
    }

    bool PopTask(AssetTaskQueue::Task& outTask)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (g_Tasks.empty())
            return false;

        outTask = g_Tasks.front();
        g_Tasks.pop();
        return true;
    }

    void WorkerLoop()
    {
        g_Running = true;

        while (!g_Stop)
        {
            AssetTaskQueue::Task task{};

            if (!PopTask(task))
            {
                g_Busy = false;
                g_Progress = 0;
                SetStatus("Idle");
                Sleep(50);
                continue;
            }

            g_Busy = true;
            g_Progress = 0;
            SetStatus("Starting", task.Name);

            if (task.Type == AssetTaskQueue::TaskType::DumpAsset)
            {
                if (!task.Assets.empty())
                {
                    SetStatus("Dumping asset", task.Assets[0].Name);
                    AssetBrowser::DumpAsset(task.Assets[0], true);
                    g_Progress = 100;
                }
            }
            else if (task.Type == AssetTaskQueue::TaskType::DumpMany)
            {
                int total = static_cast<int>(task.Assets.size());

                for (int i = 0; i < total && !g_Stop; ++i)
                {
                    const auto& asset = task.Assets[i];

                    SetStatus("Dumping assets", asset.Name);

                    AssetBrowser::DumpAsset(asset, true);

                    if (total > 0)
                        g_Progress = static_cast<int>((static_cast<double>(i + 1) / static_cast<double>(total)) * 100.0);
                }
            }
            else if (task.Type == AssetTaskQueue::TaskType::ExportCsv)
            {
                SetStatus("Exporting CSV", task.OutputPath);
                AssetBrowser::ExportCsv(task.OutputPath);
                g_Progress = 100;
            }
            else if (task.Type == AssetTaskQueue::TaskType::RebuildCache)
            {
                SetStatus("Rebuilding asset cache", task.Name);
                AssetBrowser::StartAsyncScan(true);
                AssetBrowser::SaveCache();
                g_Progress = 100;
            }
            else if (task.Type == AssetTaskQueue::TaskType::PreloadAssets)
            {
                SetStatus("Preloading assets", task.Name);
                AssetPreloadManager::StartFullPreload(false);
                g_Progress = 100;
            }

            Logger::Info("AssetTaskQueue finished task: " + task.Name);
            SetStatus("Finished", task.Name);
            g_Progress = 100;

            Sleep(200);
        }

        g_Running = false;
    }

    void PushTask(const AssetTaskQueue::Task& task)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Tasks.push(task);
    }
}

namespace AssetTaskQueue
{
    bool Init()
    {
        if (g_Running)
            return true;

        g_Stop = false;
        g_Worker = std::thread(WorkerLoop);

        Logger::Info("AssetTaskQueue initialized");
        return true;
    }

    void Shutdown()
    {
        g_Stop = true;

        if (g_Worker.joinable())
            g_Worker.join();

        {
            std::lock_guard<std::mutex> lock(g_Mutex);

            while (!g_Tasks.empty())
                g_Tasks.pop();
        }

        Logger::Info("AssetTaskQueue shutdown");
    }

    void EnqueueDumpAsset(const AssetBrowser::AssetEntry& asset)
    {
        Task task{};
        task.Type = TaskType::DumpAsset;
        task.Name = "Dump " + asset.Name;
        task.Assets.push_back(asset);
        PushTask(task);
    }

    void EnqueueDumpMany(const std::vector<AssetBrowser::AssetEntry>& assets, const std::string& name)
    {
        Task task{};
        task.Type = TaskType::DumpMany;
        task.Name = name;
        task.Assets = assets;
        PushTask(task);
    }

    void EnqueueExportCsv(const std::string& outputPath)
    {
        Task task{};
        task.Type = TaskType::ExportCsv;
        task.Name = "Export CSV";
        task.OutputPath = outputPath;
        PushTask(task);
    }

    void EnqueueRebuildCache()
    {
        Task task{};
        task.Type = TaskType::RebuildCache;
        task.Name = "Rebuild Cache";
        PushTask(task);
    }

    bool IsBusy()
    {
        return g_Busy;
    }

    int GetProgress()
    {
        return g_Progress;
    }

    std::string GetStatus()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_Status;
    }

    std::string GetCurrentTaskName()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_CurrentTaskName;
    }
}