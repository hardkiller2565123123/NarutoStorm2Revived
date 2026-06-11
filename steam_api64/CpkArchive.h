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
        uint64_t ExtractedSize = 0;

        bool Compressed = false;
        bool Heuristic = false;
        bool Extractable = false;
    };

    struct ArchiveInfo
    {
        bool Valid = false;
        std::string Status;
        std::string FullPath;
        std::vector<Entry> Entries;
    };

    bool Load(const std::string& cpkPath, ArchiveInfo& outInfo);

    bool ExtractEntry(
        const std::string& cpkPath,
        const std::string& entryPath,
        const std::string& outputPath);

    bool DumpFile(
        const std::string& cpkPath,
        const Entry& entry,
        const std::string& outputPath);
}