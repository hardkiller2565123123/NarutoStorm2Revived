#include "StdInc.h"
#include "AssetBulkExtractor.h"
#include "AssetNotification.h"
#include "Logger.h"

namespace AssetBulkExtractor
{
    bool Init(){ return true; }
    void Shutdown(){}

    int ExtractAllCpkEntries()
    {
        const auto& assets = AssetBrowser::GetAssets();
        int count = 0;

        for (const auto& asset : assets)
            if (asset.IsCpkEntry && AssetBrowser::DumpAsset(asset, true))
                count++;

        AssetNotification::PushInfo("Bulk Extract", "Extracted CPK entries: " + std::to_string(count));
        Logger::Info("BulkExtractor all CPK count=" + std::to_string(count));
        return count;
    }

    int ExtractArchive(const std::string& archivePath)
    {
        const auto& assets = AssetBrowser::GetAssets();
        int count = 0;

        for (const auto& asset : assets)
            if (asset.IsCpkEntry && asset.ArchivePath == archivePath && AssetBrowser::DumpAsset(asset, true))
                count++;

        AssetNotification::PushInfo("Bulk Extract", "Extracted archive entries: " + std::to_string(count));
        return count;
    }

    int ExtractCurrentCategory(AssetBrowser::AssetType type)
    {
        const auto& assets = AssetBrowser::GetAssets();
        int count = 0;

        for (const auto& asset : assets)
            if (asset.Type == type && AssetBrowser::DumpAsset(asset, true))
                count++;

        AssetNotification::PushInfo("Bulk Extract", "Extracted category assets: " + std::to_string(count));
        return count;
    }
}
