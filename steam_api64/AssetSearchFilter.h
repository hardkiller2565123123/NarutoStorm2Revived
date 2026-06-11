#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetSearchFilter
{
    struct Filter
    {
        std::string Search;
        std::string Extension;
        std::string ArchiveContains;

        bool OverridesOnly = false;
        bool DumpedOnly = false;
        bool ExtractableOnly = false;
        bool UnknownOnly = false;
        bool RegexMode = false;
    };

    bool Matches(const AssetBrowser::AssetEntry& asset, const Filter& filter);
}
