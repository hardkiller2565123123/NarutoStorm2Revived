#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetHash
{
    struct HashInfo
    {
        bool Valid = false;
        uint32_t CRC32 = 0;
        std::string FNV1A64;
        std::string SizeText;
    };

    bool ComputeFile(const std::string& path, HashInfo& outInfo);
    bool ComputeAsset(const AssetBrowser::AssetEntry& asset, HashInfo& outInfo);
    std::string ToHex32(uint32_t value);
    std::string BuildHashText(const HashInfo& info);
}
