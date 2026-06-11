#include "StdInc.h"
#include "AssetReport.h"
#include "AssetHash.h"
#include "AssetRelations.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace AssetReport
{
    std::string BuildOneAssetReport(const AssetBrowser::AssetEntry& asset)
    {
        std::ostringstream ss;

        ss << "Name: " << asset.Name << "\n";
        ss << "Type: " << AssetBrowser::TypeName(asset.Type) << "\n";
        ss << "Source: " << AssetBrowser::SourceName(asset.Source) << "\n";
        ss << "Virtual: " << asset.VirtualPath << "\n";
        ss << "Full: " << asset.FullPath << "\n";
        ss << "Archive: " << asset.ArchivePath << "\n";
        ss << "Entry: " << asset.ArchiveEntryPath << "\n";
        ss << "Size: " << asset.Size << "\n";
        ss << "Override: " << asset.OverrideFullPath << "\n";

        AssetHash::HashInfo hash{};
        if (AssetHash::ComputeAsset(asset, hash))
            ss << AssetHash::BuildHashText(hash);

        ss << "Dependencies:\n";
        for (const auto& dep : asset.Dependencies)
            ss << "- " << dep << "\n";

        ss << "Markers:\n";
        for (const auto& marker : asset.Markers)
            ss << "- " << marker << "\n";

        return ss.str();
    }

    bool ExportSelectedMetadata(const std::vector<AssetBrowser::AssetEntry>& assets, const std::string& outputPath)
    {
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

        std::ofstream file(outputPath, std::ios::out | std::ios::trunc);

        if (!file.is_open())
            return false;

        for (const auto& asset : assets)
        {
            file << "========================================\n";
            file << BuildOneAssetReport(asset);
            file << "\n";
        }

        return true;
    }

    bool ExportFullReport(const std::string& outputPath)
    {
        return ExportSelectedMetadata(AssetBrowser::GetAssets(), outputPath);
    }

    bool ExportDependencyReport(const std::string& outputPath)
    {
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

        std::ofstream file(outputPath, std::ios::out | std::ios::trunc);

        if (!file.is_open())
            return false;

        const auto& assets = AssetBrowser::GetAssets();

        for (const auto& asset : assets)
        {
            auto related = AssetRelations::FindRelated(asset);

            file << asset.VirtualPath << "\n";
            file << "Dependencies: " << related.Dependencies.size() << "\n";
            file << "UsedBy: " << related.UsedBy.size() << "\n";
            file << "SameFolder: " << related.SameFolder.size() << "\n\n";
        }

        return true;
    }
}
