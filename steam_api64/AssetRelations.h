#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetRelations
{
    struct RelatedResult
    {
        std::vector<int> Dependencies;
        std::vector<int> UsedBy;
        std::vector<int> SameArchive;
        std::vector<int> SameFolder;
        std::vector<int> SameType;
        std::vector<int> SameFamily;
    };

    bool Init();
    void Shutdown();

    void Rebuild();
    bool IsBuilt();

    RelatedResult FindRelated(const AssetBrowser::AssetEntry& asset);

    std::string GuessFamilyKey(const AssetBrowser::AssetEntry& asset);
}