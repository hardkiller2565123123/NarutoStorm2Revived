#pragma once
#include "StdInc.h"

namespace AssetDuplicateFinder
{
    struct DuplicateGroup
    {
        std::string Key;
        uint64_t TotalSize = 0;
        std::vector<int> AssetIndexes;
    };

    bool Init();
    void Shutdown();

    void RebuildByName();
    void RebuildBySize();
    void RebuildByHash();

    const std::vector<DuplicateGroup>& GetGroups();
    bool ExportReport(const std::string& outputPath);
}
