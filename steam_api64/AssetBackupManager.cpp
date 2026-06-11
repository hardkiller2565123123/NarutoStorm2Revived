#include "StdInc.h"
#include "AssetBackupManager.h"
#include "AssetPreviewResolver.h"
#include "AssetNotification.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <chrono>

namespace
{
    std::string TimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &t);
        char buf[64]{};
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
        return buf;
    }

    std::string Safe(std::string s)
    {
        for (char& c : s)
            if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
                c = '_';
        return s;
    }
}

namespace AssetBackupManager
{
    bool Init(){ std::filesystem::create_directories(GetBackupRoot()); return true; }
    void Shutdown(){}

    std::filesystem::path GetBackupRoot()
    {
        return std::filesystem::path(AssetBrowser::GetGameFolder()) / "NartuoStorm2Revived" / "Backups";
    }

    bool BackupAsset(const AssetBrowser::AssetEntry& asset)
    {
        std::string readable;
        if (!AssetPreviewResolver::ResolveReadablePath(asset, readable, true))
            return false;
        return BackupFile(readable, asset.Name);
    }

    bool BackupFile(const std::string& path, const std::string& label)
    {
        if (!std::filesystem::exists(path)) return false;

        std::filesystem::path out = GetBackupRoot() / (TimeStamp() + "_" + Safe(label));
        std::filesystem::create_directories(out.parent_path());

        std::error_code ec;
        std::filesystem::copy_file(path, out, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return false;

        std::ofstream manifest(GetBackupRoot() / "backup_manifest.txt", std::ios::app);
        manifest << label << "|" << path << "|" << out.string() << "|" << TimeStamp() << "\\n";

        AssetNotification::PushSuccess("Backup Created", out.string());
        Logger::Info("Backup created: " + out.string());
        return true;
    }

    bool RestoreBackup(const BackupEntry& entry)
    {
        if (!std::filesystem::exists(entry.BackupPath)) return false;

        std::filesystem::create_directories(std::filesystem::path(entry.OriginalPath).parent_path());
        std::error_code ec;
        std::filesystem::copy_file(entry.BackupPath, entry.OriginalPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return false;

        AssetNotification::PushWarning("Backup Restored", entry.OriginalPath);
        return true;
    }

    std::vector<BackupEntry> ListBackups()
    {
        std::vector<BackupEntry> out;
        std::ifstream f(GetBackupRoot() / "backup_manifest.txt");
        std::string line;

        while (std::getline(f, line))
        {
            std::stringstream ss(line);
            std::string part;
            std::vector<std::string> fields;
            while (std::getline(ss, part, '|')) fields.push_back(part);
            if (fields.size() < 4) continue;

            BackupEntry e{};
            e.Name = fields[0];
            e.OriginalPath = fields[1];
            e.BackupPath = fields[2];
            e.TimeStamp = fields[3];
            out.push_back(e);
        }
        return out;
    }

    bool ExportManifest(const std::string& outputPath)
    {
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
        std::error_code ec;
        std::filesystem::copy_file(GetBackupRoot() / "backup_manifest.txt", outputPath, std::filesystem::copy_options::overwrite_existing, ec);
        return !ec;
    }
}
