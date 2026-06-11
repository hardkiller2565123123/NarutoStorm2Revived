#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetBulkExtractor
{
    bool Init();
    void Shutdown();

    int ExtractAllCpkEntries();
    int ExtractArchive(const std::string& archivePath);
    int ExtractCurrentCategory(AssetBrowser::AssetType type);
}
