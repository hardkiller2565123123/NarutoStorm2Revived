#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"

namespace AssetHexEditor
{
    struct Session
    {
        bool Loaded = false;
        bool Dirty = false;
        std::string SourcePath;
        std::string WorkingPath;
        std::vector<unsigned char> Bytes;
        size_t Cursor = 0;
        std::string Status;
    };

    bool Init();
    void Shutdown();

    bool LoadAsset(const AssetBrowser::AssetEntry& asset, bool makeOverrideCopy);
    bool Save();
    bool SaveAs(const std::string& path);
    void Revert();
    void Clear();

    Session& GetSession();

    int FindString(const std::string& text, size_t startOffset = 0);
    std::string BuildHexView(size_t startOffset, size_t maxBytes);
}
