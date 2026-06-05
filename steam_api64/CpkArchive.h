#pragma once
#include "StdInc.h"

namespace CpkArchive
{
    struct Entry
    {
        std::string Name;
        std::string Path;
        uint64_t Offset = 0;
        uint64_t Size = 0;
        bool Heuristic = false;
    };

    struct ArchiveInfo
    {
        std::string Path;
        std::string Name;
        uint64_t Size = 0;
        std::vector<Entry> Entries;
        bool Valid = false;
        std::string Status;
    };

    bool Load(const std::string& path, ArchiveInfo& outInfo);
    bool DumpFile(const std::string& archivePath, const Entry& entry, const std::string& outPath);
    std::string BuildInfoText(const ArchiveInfo& info);
}
