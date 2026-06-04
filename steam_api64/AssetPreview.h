#pragma once
#include "StdInc.h"
#include "imgui.h"

namespace AssetPreview
{
    struct PreviewInfo
    {
        std::string Path;
        std::string Extension;
        std::string Text;
        std::string Hex;
        std::string DdsInfo;
        bool IsText = false;
        bool IsDds = false;
        bool IsModelLike = false;
        bool Loaded = false;
    };

    bool LoadPreview(const std::string& path, PreviewInfo& outInfo, size_t maxBytes = 65536);
    void DrawPreview(const PreviewInfo& info, float width, float height);
    void DrawModelPreviewPlaceholder(const char* label, float width, float height);
}
