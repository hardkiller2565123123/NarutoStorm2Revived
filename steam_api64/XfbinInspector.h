#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace XfbinInspector
{
    struct XfbinInfo
    {
        bool Valid = false;
        std::string Status;
        std::string Magic;

        std::vector<std::string> Strings;
        std::vector<std::string> Chunks;
        std::vector<std::string> TextureRefs;
        std::vector<std::string> ModelRefs;
        std::vector<std::string> MaterialRefs;
        std::vector<std::string> AnimationRefs;
        std::vector<std::string> LuaRefs;
    };

    bool Inspect(const AssetBrowser::AssetEntry& asset, XfbinInfo& outInfo);
    std::string BuildReport(const XfbinInfo& info);
}