#pragma once
#include "StdInc.h"

namespace CpkArchive
{
    struct CpkEntry
    {
        std::string ArchivePath;
        std::string Name;
        std::string Directory;
        std::string VirtualPath;
        uint64_t Offset = 0;
        uint64_t Size = 0;
        uint64_t ExtractSize = 0;
        bool Compressed = false;
        bool HasPhysicalData = false;
    };

    struct CpkInfo
    {
        std::string Path;
        std::string Name;
        uint64_t Size = 0;
        bool Parsed = false;
        std::vector<CpkEntry> Entries;
    };

    bool Load(const std::string& path, CpkInfo& outInfo);
    bool DumpEntry(const CpkEntry& entry, const std::string& dumpRoot, std::string& outPath);
    bool DumpArchive(const CpkInfo& info, const std::string& dumpRoot, std::string& outPath);
    const char* GetLastErrorText();
}
