#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetBackupManager
{
    struct BackupEntry
    {
        std::string Name;
        std::string OriginalPath;
        std::string BackupPath;
        std::string TimeStamp;
    };

    bool Init();
    void Shutdown();

    std::filesystem::path GetBackupRoot();

    bool BackupAsset(const AssetBrowser::AssetEntry& asset);
    bool BackupFile(const std::string& path, const std::string& label);
    bool RestoreBackup(const BackupEntry& entry);

    std::vector<BackupEntry> ListBackups();
    bool ExportManifest(const std::string& outputPath);
}
