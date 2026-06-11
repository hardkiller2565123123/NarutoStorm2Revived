#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetTextureInfo
{
    struct TextureInfo
    {
        bool Valid = false;
        std::string Format;
        int Width = 0;
        int Height = 0;
        int MipCount = 0;
        std::string Status;
    };

    bool Inspect(const AssetBrowser::AssetEntry& asset, TextureInfo& outInfo);
    std::string BuildText(const TextureInfo& info);
}
