#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetPreviewResolver
{
    bool ResolveReadablePath(
        const AssetBrowser::AssetEntry& asset,
        std::string& outPath,
        bool autoDumpIfNeeded = true);
}