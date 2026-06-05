#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetPreview
{
    enum class PreviewKind
    {
        None,
        Info,
        Text,
        Hex,
        TextureInfo,
        AudioInfo,
        CpkInfo,
        XfbinInfo,
        ModelShell
    };

    struct PreviewState
    {
        PreviewKind Kind = PreviewKind::None;
        std::string Title;
        std::string Text;
        std::vector<unsigned char> Bytes;
        int Width = 0;
        int Height = 0;
        int MipCount = 0;
        std::string Format;
        bool Valid = false;
        std::string Status;
    };

    bool BuildPreview(const AssetBrowser::AssetEntry& asset, PreviewState& outState, bool preferDumped = true);
    std::string BuildHex(const std::vector<unsigned char>& bytes, size_t maxBytes = 4096);
    bool ReadBytes(const std::string& path, std::vector<unsigned char>& outBytes, size_t maxBytes = 1024 * 1024);
}
