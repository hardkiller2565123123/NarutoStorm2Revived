#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetReport
{
    bool ExportSelectedMetadata(const std::vector<AssetBrowser::AssetEntry>& assets, const std::string& outputPath);
    bool ExportFullReport(const std::string& outputPath);
    bool ExportDependencyReport(const std::string& outputPath);
    std::string BuildOneAssetReport(const AssetBrowser::AssetEntry& asset);
}
