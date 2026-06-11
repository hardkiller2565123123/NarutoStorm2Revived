#pragma once
#include "StdInc.h"

namespace AssetConflictScanner
{
    struct Conflict
    {
        std::string VirtualPath;
        std::vector<std::string> Providers;
    };

    bool Init();
    void Shutdown();

    void Rebuild();
    const std::vector<Conflict>& GetConflicts();
    bool ExportReport(const std::string& outputPath);
}
